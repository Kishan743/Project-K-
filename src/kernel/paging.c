#include "paging.h"
#include "pmm.h"

#define INITIAL_IDENTITY_MAP_SIZE (32 * 1024 * 1024)
#define INITIAL_PAGE_TABLES 8

#define USER_SPACE_FIRST_DIRECTORY 256

#define PAGING_MAX_ADDRESS_SPACES 16

static uint32_t page_directory[1024]
    __attribute__((aligned(PAGE_SIZE)));

static uint32_t initial_page_tables[INITIAL_PAGE_TABLES][1024]
    __attribute__((aligned(PAGE_SIZE)));

static uint32_t paging_directory_address;
static uint32_t current_directory_address;

static address_space_t kernel_address_space;

static address_space_t address_spaces[
    PAGING_MAX_ADDRESS_SPACES
];

static uint8_t address_space_used[
    PAGING_MAX_ADDRESS_SPACES
];

static inline void paging_load_directory(
    uint32_t physical_address
)
{
    __asm__ volatile (
        "mov %0, %%cr3"
        :
        : "r"(physical_address)
        : "memory"
    );
}

static inline void paging_enable(void)
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

static uint32_t* directory_from_address(
    uint32_t address
)
{
    if (address >= INITIAL_IDENTITY_MAP_SIZE)
        return 0;

    return (uint32_t*)address;
}

static uint32_t* page_table_from_directory(
    uint32_t directory_entry
)
{
    uint32_t table_address =
        directory_entry & 0xFFFFF000;

    if (table_address >= INITIAL_IDENTITY_MAP_SIZE)
        return 0;

    return (uint32_t*)table_address;
}

static int create_page_table(
    uint32_t directory_address,
    uint32_t directory_index,
    uint32_t* table_address_out
)
{
    uint32_t* directory =
        directory_from_address(
            directory_address
        );

    if (directory == 0)
        return -1;

    void* frame =
        pmm_alloc_frame();

    if (frame == 0)
        return -1;

    uint32_t physical =
        (uint32_t)frame;

    /*
     * Page tables are currently accessed by the kernel
     * through the identity mapping, so they must live
     * below the identity-mapped 32 MiB region.
     */
    if (physical >= INITIAL_IDENTITY_MAP_SIZE)
    {
        pmm_free_frame(frame);
        return -1;
    }

    uint32_t* table =
        (uint32_t*)physical;

    for (uint32_t i = 0;
         i < 1024;
         i++)
    {
        table[i] = 0;
    }

    directory[directory_index] =
        physical |
        PAGE_PRESENT |
        PAGE_WRITABLE;

    if (table_address_out != 0)
        *table_address_out = physical;

    return 0;
}

static int map_page_in_directory(
    uint32_t directory_address,
    uint32_t virtual_address,
    uint32_t physical_address,
    uint32_t flags
)
{
    uint32_t* directory =
        directory_from_address(
            directory_address
        );

    if (directory == 0)
        return -1;

    uint32_t directory_index =
        (virtual_address >> 22) & 0x3FF;

    uint32_t table_index =
        (virtual_address >> 12) & 0x3FF;

    uint32_t directory_entry =
        directory[directory_index];

    uint32_t* page_table;

    if ((directory_entry & PAGE_PRESENT) == 0)
    {
        uint32_t table_address;

        if (create_page_table(
                directory_address,
                directory_index,
                &table_address
            ) != 0)
        {
            return -1;
        }

        page_table =
            (uint32_t*)table_address;

        directory_entry =
            directory[directory_index];
    }
    else
    {
        page_table =
            page_table_from_directory(
                directory_entry
            );

        if (page_table == 0)
            return -1;
    }

    if (flags & PAGE_USER)
    {
        directory[directory_index] |= PAGE_USER;
    }

    page_table[table_index] =
        (physical_address & 0xFFFFF000) |
        (flags & 0xFFF) |
        PAGE_PRESENT;

    __asm__ volatile (
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );

    return 0;
}

static void mark_owned_page_table(
    address_space_t* space,
    uint32_t directory_index
)
{
    if (space == 0)
        return;

    if (directory_index >= 1024)
        return;

    uint32_t word =
        directory_index / 32;

    uint32_t bit =
        directory_index % 32;

    space->owned_page_tables[word] |=
        (1u << bit);
}

static int owns_page_table(
    address_space_t* space,
    uint32_t directory_index
)
{
    if (space == 0)
        return 0;

    if (directory_index >= 1024)
        return 0;

    uint32_t word =
        directory_index / 32;

    uint32_t bit =
        directory_index % 32;

    return (
        space->owned_page_tables[word] &
        (1u << bit)
    ) != 0;
}

void paging_initialize(void)
{
    for (uint32_t i = 0;
         i < 1024;
         i++)
    {
        page_directory[i] = 0;
    }

    for (uint32_t table = 0;
         table < INITIAL_PAGE_TABLES;
         table++)
    {
        for (uint32_t entry = 0;
             entry < 1024;
             entry++)
        {
            uint32_t physical =
                (
                    table * 1024 +
                    entry
                ) * PAGE_SIZE;

            initial_page_tables[table][entry] =
                physical |
                PAGE_PRESENT |
                PAGE_WRITABLE;
        }

        page_directory[table] =
            (uint32_t)&initial_page_tables[table][0] |
            PAGE_PRESENT |
            PAGE_WRITABLE;
    }

    paging_directory_address =
        (uint32_t)&page_directory[0];

    current_directory_address =
        paging_directory_address;

    kernel_address_space.page_directory =
        paging_directory_address;

    for (uint32_t i = 0;
         i < PAGING_MAX_USER_PAGE_TABLES / 32;
         i++)
    {
        kernel_address_space.owned_page_tables[i] = 0;
    }

    for (uint32_t i = 0;
         i < PAGING_MAX_ADDRESS_SPACES;
         i++)
    {
        address_space_used[i] = 0;

        address_spaces[i].page_directory = 0;

        for (uint32_t word = 0;
             word < PAGING_MAX_USER_PAGE_TABLES / 32;
             word++)
        {
            address_spaces[i].owned_page_tables[word] = 0;
        }
    }

    paging_load_directory(
        paging_directory_address
    );

    paging_enable();
}

int paging_map_page(
    uint32_t virtual_address,
    uint32_t physical_address,
    uint32_t flags
)
{
    return map_page_in_directory(
        paging_directory_address,
        virtual_address,
        physical_address,
        flags
    );
}

int paging_unmap_page(
    uint32_t virtual_address
)
{
    uint32_t* directory =
        directory_from_address(
            paging_directory_address
        );

    if (directory == 0)
        return -1;

    uint32_t directory_index =
        (virtual_address >> 22) & 0x3FF;

    uint32_t table_index =
        (virtual_address >> 12) & 0x3FF;

    uint32_t directory_entry =
        directory[directory_index];

    if ((directory_entry & PAGE_PRESENT) == 0)
        return -1;

    uint32_t* page_table =
        page_table_from_directory(
            directory_entry
        );

    if (page_table == 0)
        return -1;

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
            continue;

        void* physical_frame =
            pmm_alloc_frame();

        if (physical_frame == 0)
            return 0;

        uint32_t directory_address =
            (uint32_t)physical_frame;

        if (directory_address >= INITIAL_IDENTITY_MAP_SIZE)
        {
            pmm_free_frame(
                physical_frame
            );

            return 0;
        }

        uint32_t* directory =
            (uint32_t*)directory_address;

        for (uint32_t entry = 0;
             entry < 1024;
             entry++)
        {
            directory[entry] = 0;
        }

        /*
         * Share every existing kernel mapping.
         *
         * Kernel mappings remain supervisor-only.
         *
         * The PDE's USER bit is explicitly cleared so
         * Ring 3 cannot access the shared kernel mapping.
         */
        for (uint32_t entry = 0;
             entry < 1024;
             entry++)
        {
            directory[entry] =
                page_directory[entry] &
                ~PAGE_USER;
        }

        address_spaces[i].page_directory =
            directory_address;

        for (uint32_t word = 0;
             word < PAGING_MAX_USER_PAGE_TABLES / 32;
             word++)
        {
            address_spaces[i].owned_page_tables[word] = 0;
        }

        address_space_used[i] = 1;

        return &address_spaces[i];
    }

    return 0;
}

int paging_map_user_page(
    address_space_t* space,
    uint32_t virtual_address,
    uint32_t physical_address,
    uint32_t flags
)
{
    if (space == 0)
        return -1;

    uint32_t directory_index =
        (virtual_address >> 22) & 0x3FF;

    if (directory_index < USER_SPACE_FIRST_DIRECTORY)
        return -1;

    uint32_t* directory =
        directory_from_address(
            space->page_directory
        );

    if (directory == 0)
        return -1;

    uint32_t had_page_table =
        directory[directory_index] &
        PAGE_PRESENT;

    flags |= PAGE_USER;
    flags |= PAGE_PRESENT;

    int result =
        map_page_in_directory(
            space->page_directory,
            virtual_address,
            physical_address,
            flags
        );

    if (result != 0)
        return result;

    /*
     * If this mapping caused creation of a new page table,
     * that page table belongs to this address space.
     */
    if (!had_page_table)
    {
        mark_owned_page_table(
            space,
            directory_index
        );
    }

    return 0;
}

void paging_switch_address_space(
    address_space_t* space
)
{
    if (space == 0)
        return;

    if (space->page_directory == 0)
        return;

    current_directory_address =
        space->page_directory;

    paging_load_directory(
        space->page_directory
    );
}

void paging_destroy_address_space(
    address_space_t* space
)
{
    if (space == 0)
        return;

    if (space == &kernel_address_space)
        return;

    if (space->page_directory == 0)
        return;

    /*
     * Never destroy the address space while it is active.
     */
    if (current_directory_address ==
        space->page_directory)
    {
        paging_switch_address_space(
            &kernel_address_space
        );
    }

    uint32_t* directory =
        directory_from_address(
            space->page_directory
        );

    if (directory == 0)
        return;

    /*
     * Only free page tables created by this address space.
     *
     * Shared kernel page tables remain owned by the kernel.
     */
    for (uint32_t directory_index = 0;
         directory_index < 1024;
         directory_index++)
    {
        if (!owns_page_table(
                space,
                directory_index))
        {
            continue;
        }

        uint32_t entry =
            directory[directory_index];

        if ((entry & PAGE_PRESENT) == 0)
            continue;

        uint32_t table_address =
            entry & 0xFFFFF000;

        if (table_address <
            INITIAL_IDENTITY_MAP_SIZE)
        {
            pmm_free_frame(
                (void*)table_address
            );
        }

        directory[directory_index] = 0;
    }

    /*
     * Release the private page-directory frame.
     */
    uint32_t directory_address =
        space->page_directory;

    if (directory_address <
        INITIAL_IDENTITY_MAP_SIZE)
    {
        pmm_free_frame(
            (void*)directory_address
        );
    }

    space->page_directory = 0;

    for (uint32_t word = 0;
         word < PAGING_MAX_USER_PAGE_TABLES / 32;
         word++)
    {
        space->owned_page_tables[word] = 0;
    }

    for (uint32_t i = 0;
         i < PAGING_MAX_ADDRESS_SPACES;
         i++)
    {
        if (&address_spaces[i] == space)
        {
            address_space_used[i] = 0;
            break;
        }
    }
}