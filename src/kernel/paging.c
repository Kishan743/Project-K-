#include "paging.h"
#include "pmm.h"

#define PAGE_PRESENT  0x001
#define PAGE_WRITABLE 0x002

/*
 * Project K initially keeps the first 32 MiB identity-mapped.
 *
 * This gives the kernel direct access to physical frames in
 * this range while the paging subsystem is being developed.
 */
#define INITIAL_IDENTITY_MAP_SIZE (32 * 1024 * 1024)
#define INITIAL_PAGE_TABLES \
    (INITIAL_IDENTITY_MAP_SIZE / (1024 * PAGE_SIZE))

static uint32_t page_directory[1024]
    __attribute__((aligned(4096)));

static uint32_t paging_directory_address;

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
 * Return a pointer to a page table.
 *
 * Project K currently identity-maps the first 32 MiB,
 * so PMM frames used for page tables must initially come
 * from that accessible range.
 */
static uint32_t* page_table_from_directory(uint32_t directory_index)
{
    if (directory_index >= 1024)
    {
        return 0;
    }

    if ((page_directory[directory_index] & PAGE_PRESENT) == 0)
    {
        return 0;
    }

    uint32_t physical_address =
        page_directory[directory_index] & 0xFFFFF000;

    /*
     * The page table must currently be inside the
     * identity-mapped 32 MiB region.
     */
    if (physical_address >= INITIAL_IDENTITY_MAP_SIZE)
    {
        return 0;
    }

    return (uint32_t*)physical_address;
}

/*
 * Create a page table for a page-directory entry.
 *
 * Returns the virtual address of the page table, or 0
 * on failure.
 */
static uint32_t* create_page_table(uint32_t directory_index)
{
    void* physical_frame =
        pmm_alloc_frame();

    if (physical_frame == 0)
    {
        return 0;
    }

    uint32_t physical_address =
        (uint32_t)physical_frame;

    /*
     * We need to access the new page table immediately.
     *
     * For now, the PMM must provide a frame inside our
     * identity-mapped 32 MiB region.
     */
    if (physical_address >= INITIAL_IDENTITY_MAP_SIZE)
    {
        pmm_free_frame(physical_frame);
        return 0;
    }

    uint32_t* table =
        (uint32_t*)physical_address;

    /*
     * Clear all 1024 page-table entries.
     */
    for (uint32_t i = 0; i < 1024; i++)
    {
        table[i] = 0;
    }

    page_directory[directory_index] =
        physical_address |
        PAGE_PRESENT |
        PAGE_WRITABLE;

    return table;
}

void paging_initialize(void)
{
    /*
     * Clear the page directory.
     */
    for (uint32_t i = 0; i < 1024; i++)
    {
        page_directory[i] = 0;
    }

    /*
     * Dynamically allocate page tables from the PMM
     * for the initial 32 MiB identity mapping.
     */
    for (uint32_t table = 0;
         table < INITIAL_PAGE_TABLES;
         table++)
    {
        uint32_t* page_table =
            create_page_table(table);

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

    paging_load_directory(
        paging_directory_address
    );

    paging_enable();
}

int paging_map_page(uint32_t virtual_address,
                    uint32_t physical_address,
                    uint32_t flags)
{
    uint32_t directory_index =
        (virtual_address >> 22) & 0x3FF;

    uint32_t table_index =
        (virtual_address >> 12) & 0x3FF;

    uint32_t* page_table =
        page_table_from_directory(directory_index);

    /*
     * If the page table does not exist, create it.
     */
    if (page_table == 0)
    {
        page_table =
            create_page_table(directory_index);

        if (page_table == 0)
        {
            return -1;
        }
    }

    page_table[table_index] =
        (physical_address & 0xFFFFF000) |
        (flags & 0xFFF);

    /*
     * Invalidate the TLB entry for this virtual address.
     */
    __asm__ volatile (
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );

    return 0;
}

int paging_unmap_page(uint32_t virtual_address)
{
    uint32_t directory_index =
        (virtual_address >> 22) & 0x3FF;

    uint32_t table_index =
        (virtual_address >> 12) & 0x3FF;

    uint32_t* page_table =
        page_table_from_directory(directory_index);

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
    return paging_directory_address;
}
