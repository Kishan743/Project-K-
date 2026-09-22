#include "heap.h"
#include "pmm.h"
#include "paging.h"

#define HEAP_START       0x01000000
#define HEAP_SIZE        (4 * 1024 * 1024)
#define HEAP_ALIGNMENT   8
#define HEAP_MIN_SPLIT   16

typedef struct heap_block
{
    uint32_t size;
    uint8_t  free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_first;
static uint32_t heap_used;
static uint32_t heap_committed;

static uint32_t align_up(uint32_t value)
{
    return (value + (HEAP_ALIGNMENT - 1)) &
           ~(HEAP_ALIGNMENT - 1);
}

static uint32_t page_align_up(uint32_t value)
{
    return (value + PAGE_SIZE - 1) &
           ~(PAGE_SIZE - 1);
}

static heap_block_t* find_free_block(uint32_t size)
{
    heap_block_t* current = heap_first;

    while (current != 0)
    {
        if (current->free && current->size >= size)
            return current;

        current = current->next;
    }

    return 0;
}

static heap_block_t* get_last_block(void)
{
    heap_block_t* current = heap_first;

    if (current == 0)
        return 0;

    while (current->next != 0)
        current = current->next;

    return current;
}

static void split_block(heap_block_t* block,
                        uint32_t size)
{
    if (block->size < size)
        return;

    uint32_t remaining =
        block->size - size;

    if (remaining <
        sizeof(heap_block_t) + HEAP_MIN_SPLIT)
        return;

    heap_block_t* new_block =
        (heap_block_t*)
        ((uint8_t*)block +
         sizeof(heap_block_t) +
         size);

    new_block->size =
        remaining - sizeof(heap_block_t);

    new_block->free = 1;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}

static void coalesce_free_blocks(void)
{
    heap_block_t* current = heap_first;

    while (current != 0 &&
           current->next != 0)
    {
        heap_block_t* next =
            current->next;

        if (current->free && next->free)
        {
            current->size +=
                sizeof(heap_block_t) +
                next->size;

            current->next = next->next;
            continue;
        }

        current = current->next;
    }
}

static int commit_pages(uint32_t required_end)
{
    uint32_t required_committed =
        page_align_up(required_end);

    if (required_committed > HEAP_SIZE)
        return -1;

    if (required_committed <= heap_committed)
        return 0;

    uint32_t old_committed =
        heap_committed;

    uint32_t additional =
        required_committed - old_committed;

    uint32_t pages =
        additional / PAGE_SIZE;

    void* frames[1024];

    if (pages > 1024)
        return -1;

    uint32_t allocated = 0;

    /*
     * Allocate all frames first.
     */
    for (uint32_t i = 0;
         i < pages;
         i++)
    {
        frames[i] = pmm_alloc_frame();

        if (frames[i] == 0)
        {
            for (uint32_t j = 0;
                 j < allocated;
                 j++)
            {
                pmm_free_frame(frames[j]);
            }

            return -1;
        }

        allocated++;
    }

    /*
     * Map the new virtual pages.
     */
    for (uint32_t i = 0;
         i < pages;
         i++)
    {
        uint32_t virtual_address =
            HEAP_START +
            old_committed +
            i * PAGE_SIZE;

        if (paging_map_page(
                virtual_address,
                (uint32_t)frames[i],
                PAGE_PRESENT | PAGE_WRITABLE) != 0)
        {
            /*
             * Roll back mappings already installed.
             */
            for (uint32_t j = 0;
                 j < i;
                 j++)
            {
                paging_unmap_page(
                    HEAP_START +
                    old_committed +
                    j * PAGE_SIZE
                );

                pmm_free_frame(frames[j]);
            }

            for (uint32_t j = i;
                 j < allocated;
                 j++)
            {
                pmm_free_frame(frames[j]);
            }

            return -1;
        }
    }

    heap_committed =
        required_committed;

    return 0;
}

void heap_initialize(void)
{
    heap_first = 0;
    heap_used = 0;
    heap_committed = 0;
}

void* kmalloc(uint32_t size)
{
    if (size == 0)
        return 0;

    size = align_up(size);

    /*
     * First try an existing free block.
     */
    heap_block_t* block =
        find_free_block(size);

    if (block != 0)
    {
        split_block(block, size);

        block->free = 0;
        heap_used += block->size;

        return (void*)
            ((uint8_t*)block +
             sizeof(heap_block_t));
    }

    /*
     * Append a new block to the logical heap.
     */
    heap_block_t* last =
        get_last_block();

    uint32_t block_address;

    if (last == 0)
    {
        block_address =
            HEAP_START;
    }
    else
    {
        block_address =
            (uint32_t)last +
            sizeof(heap_block_t) +
            last->size;
    }

    uint32_t required_end =
        (block_address - HEAP_START) +
        sizeof(heap_block_t) +
        size;

    if (commit_pages(required_end) != 0)
        return 0;

    block =
        (heap_block_t*)block_address;

    block->size = size;
    block->free = 0;
    block->next = 0;

    if (last == 0)
        heap_first = block;
    else
        last->next = block;

    heap_used += size;

    return (void*)
        ((uint8_t*)block +
         sizeof(heap_block_t));
}

void kfree(void* pointer)
{
    if (pointer == 0)
        return;

    heap_block_t* block =
        (heap_block_t*)
        ((uint8_t*)pointer -
         sizeof(heap_block_t));

    if (block->free)
        return;

    block->free = 1;

    if (heap_used >= block->size)
        heap_used -= block->size;
    else
        heap_used = 0;

    coalesce_free_blocks();
}

uint32_t heap_get_used(void)
{
    return heap_used;
}

uint32_t heap_get_free(void)
{
    if (heap_used >= HEAP_SIZE)
        return 0;

    return HEAP_SIZE - heap_used;
}
