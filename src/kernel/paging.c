#include "paging.h"
#include "pmm.h"

#define INITIAL_IDENTITY_MAP_SIZE (32 * 1024 * 1024)
#define INITIAL_PAGE_TABLES \
    (INITIAL_IDENTITY_MAP_SIZE / (1024 * PAGE_SIZE))

#define USER_SPACE_FIRST_DIRECTORY 256
#define PAGING_MAX_ADDRESS_SPACES 16

static uint32_t page_directory[1024]
    __attribute__((aligned(4096)));

static uint32_t paging_directory_address;
static uint32_t current_directory_address;

static address_space_t kernel_address_space;

static address_space_t address_spaces[PAGING_MAX_ADDRESS_SPACES];
static uint8_t address_space_used[PAGING_MAX_ADDRESS_SPACES];

static void paging_load_directory(uint32_t address)
{
    __asm__ volatile (
        "mov %0, %%cr3"
        :
        : "r"(address)
        : "memory"
    );
}

static void paging_enable(void)
{
    uint32_t cr0;

    __asm__ volatile (
        "mov %%cr0, %0"
        : "=r"(cr0)
    );

    cr0 |= 0x80000000;

    __asm__ volatile (
        "mov %0, %%cr0"
        :
        : "r"(cr0)
        : "memory"
    );
}

/*
 * Convert a physical page-directory address into a pointer.
 *
 * Project K currently identity-maps the first 32 MiB,
 * so all page directories and page tables used here must
 * reside inside that region.
 */
static uint32_t* directory_from_address(uint32_t directory_address)
{
    if (directory_address == 0)
    {
        return 0;
    }

    if (directory_address >= INITIAL_IDENTITY_MAP_SIZE)
    {
        return 0;
    }

    return (uint32_t*)directory_address;
}

/*
 * Return the page table belonging to a page directory entry.
 */
static uint32_t* page_table_from_directory(
    uint32_t directory_address,
    uint32_t directory_index)
{
    if (directory_index >= 1024)
    {
        return 0;
    }

    uint32_t* directory =
        directory_from_address(directory_address);

    if (directory == 0)
    {
        return 0;
    }

    if ((directory[directory_index] & PAGE_PRESENT) == 0)
    {
        return 0;
    }

    uint32_t physical_address =
        directory[directory_index] & 0xFFFFF000;

    if (physical_address >= INITIAL_IDENTITY_MAP_SIZE)
    {
        return 0;
    }

    return (uint32_t*)physical_address;
}

/*
 * Create a new page table inside a specific address space.
 */
static uint32_t* create_page_table(
    uint32_t directory_address,
    uint32_t directory_index)
{
    if (directory_index >= 1024)
    {
        return 0;
    }

    uint32_t* directory =
        directory_from_address(directory_address);

    if (directory == 0)
    {
        return 0;
    }

    void* physical_frame =
        pmm_alloc_frame();

    if (physical_frame == 0)
    {
        return 0;
    }

    uint32_t physical_address =
        (uint32_t)physical_frame;

    /*
     * The current kernel can directly access only the
     * identity-mapped first 32 MiB.
     */
    if (physical_address >= INITIAL_IDENTITY_MAP_SIZE)
    {
        pmm_free_frame(physical_frame);
        return 0;
    }

    uint32_t* table =
        (uint32_t*)physical_address;

    for (uint32_t i = 0; i < 1024; i++)
    {
        table[i] = 0;
    }

    directory[directory_index] =
        physical_address |
        PAGE_PRESENT |
        PAGE_WRITABLE;

    return table;
}

/*
 * Map one page into a specified address space.
 */
static int map_page_in_directory(
    uint32_t directory_address,
    uint32_t virtual_address,
    uint32_t physical_address,
    uint32_t flags)
{
    uint32_t directory_index =
        (virtual_address >> 22) & 0x3FF;

    uint32_t table_index =
        (virtual_address >> 12) & 0x3FF;

    uint32_t* page_table =
        page_table_from_directory(
            directory_address,
            directory_index
        );

    if (page_table == 0)
    {
        page_table =
            create_page_table(
                directory_address,
                directory_index
            );

        if (page_table == 0)
        {
            return -1;
        }
    }

    uint32_t* directory =
        directory_from_address(directory_address);

    if (directory == 0)
    {
        return -1;
    }

    /*
     * User access requires PAGE_USER in both the PDE
     * and the PTE.
     */
    if (flags & PAGE_USER)
    {
        directory[directory_index] |= PAGE_USER;
    }

    page_table[table_index] =
        (physical_address & 0xFFFFF000) |
        (flags & 0xFFF);

    /*
     * This affects the currently active address space.
     * CR3 switching later provides a complete TLB refresh
     * when another address space becomes active.
     */
    __asm__ volatile (
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );

    return 0;
}

void paging_initialize(void)
{
    /*
     * Clear the kernel page directory.
     */
    for (uint32_t i = 0; i < 1024; i++)
    {
        page_directory[i] = 0;
    }

    /*
     * Dynamically allocate page tables for the initial
     * 32 MiB identity mapping.
     */
    for (uint32_t table = 0;
         table < INITIAL_PAGE_TABLES;
         table++)
    {
        uint32_t* page_table =
            create_page_table(
                (uint32_t)page_directory,
                table
            );

        if (page_table == 0)
        {
            /*
             * Paging initialization cannot continue
             * without the initial page tables.
             */
            return;
        }

        /*
         * Identity-map 4 MiB using this page table.
         */
        for (uint32_t page = 0;
             page < 1024;
             page++)
        {
            uint32_t physical =
                (table * 1024 + page) * PAGE_SIZE;

            page_table[page] =
                physical |
                PAGE_PRESENT |
                PAGE_WRITABLE;
        }
    }

    paging_directory_address =
        (uint32_t)page_directory;

    current_directory_address =
        paging_directory_address;

    kernel_address_space.page_directory =
        paging_directory_address;

    paging_load_directory(
        paging_directory_address
    );

    paging_enable();
}

int paging_map_page(uint32_t virtual_address,
                    uint32_t physical_address,
                    uint32_t flags)
{
    return map_page_in_directory(
        paging_directory_address,
        virtual_address,
        physical_address,
        flags
    );
}

int paging_unmap_page(uint32_t virtual_address)
{
    uint32_t directory_index =
        (virtual_address >> 22) & 0x3FF;

    uint32_t table_index =
        (virtual_address >> 12) & 0x3FF;

    uint32_t* page_table =
        page_table_from_directory(
            paging_directory_address,
            directory_index
        );

    if (page_table == 0)
    {
        return -1;
    }

    page_table[table_index] = 0;

    __asm__ volatile (
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );

    return 0;
}

uint32_t paging_get_directory(void)
{
    return current_directory_address;
}

address_space_t* paging_get_kernel_address_space(void)
{
    return &kernel_address_space;
}

address_space_t* paging_create_address_space(void)
{
    for (uint32_t i = 0;
         i < PAGING_MAX_ADDRESS_SPACES;
         i++)
    {
        if (address_space_used[i])
        {
            continue;
        }

        void* physical_frame =
            pmm_alloc_frame();

        if (physical_frame == 0)
        {
            return 0;
        }

        uint32_t directory_address =
            (uint32_t)physical_frame;

        if (directory_address >= INITIAL_IDENTITY_MAP_SIZE)
        {
            pmm_free_frame(physical_frame);
            return 0;
        }

        uint32_t* directory =
            (uint32_t*)directory_address;

        /*
         * Start with an empty page directory.
         */
        for (uint32_t entry = 0;
             entry < 1024;
             entry++)
        {
            directory[entry] = 0;
        }

        /*
         * Share the kernel's existing page tables.
         *
         * The page tables themselves remain owned by the
         * kernel address space. We only copy the PDE values.
         */
        for (uint32_t entry = 0;
             entry < USER_SPACE_FIRST_DIRECTORY;
             entry++)
        {
            directory[entry] =
                page_directory[entry];
        }

        address_spaces[i].page_directory =
            directory_address;

        address_space_used[i] = 1;

        return &address_spaces[i];
    }

    return 0;
}

int paging_map_user_page(address_space_t* space,
                         uint32_t virtual_address,
                         uint32_t physical_address,
                         uint32_t flags)
{
    if (space == 0)
    {
        return -1;
    }

    /*
     * Keep the user portion separate from the shared
     * kernel portion of the address space.
     */
    uint32_t directory_index =
        (virtual_address >> 22) & 0x3FF;

    if (directory_index < USER_SPACE_FIRST_DIRECTORY)
    {
        return -1;
    }

    flags |= PAGE_USER;
    flags |= PAGE_PRESENT;

    return map_page_in_directory(
        space->page_directory,
        virtual_address,
        physical_address,
        flags
    );
}

void paging_switch_address_space(address_space_t* space)
{
    if (space == 0)
    {
        return;
    }

    if (space->page_directory == 0)
    {
        return;
    }

    current_directory_address =
        space->page_directory;

    paging_load_directory(
        space->page_directory
    );
}

void paging_destroy_address_space(address_space_t* space)
{
    if (space == 0)
    {
        return;
    }

    if (space == &kernel_address_space)
    {
        return;
    }

    if (space->page_directory == 0)
    {
        return;
    }

    uint32_t directory_address =
        space->page_directory;

    uint32_t* directory =
        directory_from_address(
            directory_address
        );

    if (directory == 0)
    {
        return;
    }

    /*
     * User page tables are created only in the user
     * portion of the address space.
     *
     * Do NOT free the kernel page tables because they
     * are shared with the kernel address space.
     */
    for (uint32_t entry = USER_SPACE_FIRST_DIRECTORY;
         entry < 1024;
         entry++)
    {
        if ((directory[entry] & PAGE_PRESENT) == 0)
        {
            continue;
        }

        uint32_t page_table_address =
            directory[entry] & 0xFFFFF000;

        if (page_table_address <
            INITIAL_IDENTITY_MAP_SIZE)
        {
            pmm_free_frame(
                (void*)page_table_address
            );
        }

        directory[entry] = 0;
    }

    /*
     * The currently active address space must never be
     * destroyed while it is active.
     */
    if (current_directory_address ==
        directory_address)
    {
        paging_switch_address_space(
            &kernel_address_space
        );
    }

    pmm_free_frame(
        (void*)directory_address
    );

    for (uint32_t i = 0;
         i < PAGING_MAX_ADDRESS_SPACES;
         i++)
    {
        if (&address_spaces[i] == space)
        {
            address_spaces[i].page_directory = 0;
            address_space_used[i] = 0;
            break;
        }
    }
}