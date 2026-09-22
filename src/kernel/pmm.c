#include "pmm.h"
#include "multiboot.h"

#define MAX_MEMORY_BYTES (128 * 1024 * 1024)
#define MAX_FRAMES (MAX_MEMORY_BYTES / PMM_PAGE_SIZE)

extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

static uint8_t frame_bitmap[MAX_FRAMES / 8];
static uint8_t reserved_bitmap[MAX_FRAMES / 8];

static uint32_t total_frames;
static uint32_t free_frames;

static void bitmap_set(uint8_t* bitmap, uint32_t frame)
{
    bitmap[frame / 8] |=
        (uint8_t)(1u << (frame % 8));
}

static void bitmap_clear(uint8_t* bitmap, uint32_t frame)
{
    bitmap[frame / 8] &=
        (uint8_t)~(1u << (frame % 8));
}

static int bitmap_test(const uint8_t* bitmap,
                       uint32_t frame)
{
    return (bitmap[frame / 8] &
            (uint8_t)(1u << (frame % 8))) != 0;
}

static void mark_range_free(uint32_t base,
                            uint32_t length)
{
    uint32_t start =
        (base + PMM_PAGE_SIZE - 1) &
        ~(PMM_PAGE_SIZE - 1);

    uint32_t end =
        (base + length) &
        ~(PMM_PAGE_SIZE - 1);

    if (end <= start)
        return;

    uint32_t first_frame =
        start / PMM_PAGE_SIZE;

    uint32_t last_frame =
        end / PMM_PAGE_SIZE;

    if (first_frame >= MAX_FRAMES)
        return;

    if (last_frame > MAX_FRAMES)
        last_frame = MAX_FRAMES;

    for (uint32_t frame = first_frame;
         frame < last_frame;
         frame++)
    {
        if (bitmap_test(frame_bitmap, frame))
        {
            bitmap_clear(frame_bitmap, frame);
            free_frames++;
        }
    }
}

static void mark_range_used(uint32_t base,
                            uint32_t length)
{
    uint32_t start =
        base & ~(PMM_PAGE_SIZE - 1);

    uint32_t end =
        (base + length + PMM_PAGE_SIZE - 1) &
        ~(PMM_PAGE_SIZE - 1);

    if (end <= start)
        return;

    uint32_t first_frame =
        start / PMM_PAGE_SIZE;

    uint32_t last_frame =
        end / PMM_PAGE_SIZE;

    if (first_frame >= MAX_FRAMES)
        return;

    if (last_frame > MAX_FRAMES)
        last_frame = MAX_FRAMES;

    for (uint32_t frame = first_frame;
         frame < last_frame;
         frame++)
    {
        if (!bitmap_test(frame_bitmap, frame))
        {
            bitmap_set(frame_bitmap, frame);
            free_frames--;
        }

        bitmap_set(reserved_bitmap, frame);
    }
}

static void parse_memory_map(multiboot2_info_t* info)
{
    uint8_t* current =
        (uint8_t*)info + 8;

    uint8_t* end =
        (uint8_t*)info + info->total_size;

    while (current < end)
    {
        multiboot2_tag_t* tag =
            (multiboot2_tag_t*)current;

        if (tag->type == MULTIBOOT2_TAG_MMAP)
        {
            multiboot2_tag_mmap_t* mmap =
                (multiboot2_tag_mmap_t*)tag;

            uint8_t* entry_address =
                current + sizeof(multiboot2_tag_mmap_t);

            uint8_t* mmap_end =
                current + mmap->size;

            while (entry_address < mmap_end)
            {
                multiboot2_mmap_entry_t* entry =
                    (multiboot2_mmap_entry_t*)entry_address;

                if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE &&
                    entry->base_addr <= 0xFFFFFFFFULL &&
                    entry->length > 0)
                {
                    uint64_t end_address =
                        entry->base_addr + entry->length;

                    if (end_address > entry->base_addr &&
                        end_address <= 0x100000000ULL)
                    {
                        uint32_t base =
                            (uint32_t)entry->base_addr;

                        uint32_t length =
                            (uint32_t)entry->length;

                        mark_range_free(base, length);
                    }
                }

                entry_address += mmap->entry_size;
            }
        }

        if (tag->type == MULTIBOOT2_TAG_END)
            break;

        current += (tag->size + 7) & ~7u;
    }
}

void pmm_initialize(uint32_t multiboot_info)
{
    multiboot2_info_t* info =
        (multiboot2_info_t*)multiboot_info;

    total_frames = MAX_FRAMES;
    free_frames = 0;

    for (uint32_t i = 0;
         i < MAX_FRAMES / 8;
         i++)
    {
        frame_bitmap[i] = 0xFF;
        reserved_bitmap[i] = 0x00;
    }

    parse_memory_map(info);

    /*
     * Reserve physical address zero.
     */
    mark_range_used(
        0,
        PMM_PAGE_SIZE
    );

    /*
     * Reserve the Multiboot 2 information structure.
     */
    mark_range_used(
        multiboot_info,
        info->total_size
    );

    /*
     * Reserve the exact Project K kernel image.
     */
    uint32_t kernel_start =
        (uint32_t)&__kernel_start;

    uint32_t kernel_end =
        (uint32_t)&__kernel_end;

    mark_range_used(
        kernel_start,
        kernel_end - kernel_start
    );
}

uint32_t pmm_get_total_frames(void)
{
    return total_frames;
}

uint32_t pmm_get_free_frames(void)
{
    return free_frames;
}

void* pmm_alloc_frame(void)
{
    for (uint32_t frame = 0;
         frame < total_frames;
         frame++)
    {
        if (!bitmap_test(frame_bitmap, frame))
        {
            bitmap_set(frame_bitmap, frame);
            free_frames--;

            return (void*)(frame * PMM_PAGE_SIZE);
        }
    }

    return 0;
}

void pmm_free_frame(void* address)
{
    uint32_t frame =
        (uint32_t)address / PMM_PAGE_SIZE;

    if (frame >= total_frames)
        return;

    if (bitmap_test(reserved_bitmap, frame))
        return;

    if (bitmap_test(frame_bitmap, frame))
    {
        bitmap_clear(frame_bitmap, frame);
        free_frames++;
    }
}
