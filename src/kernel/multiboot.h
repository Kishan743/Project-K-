#ifndef PROJECT_K_MULTIBOOT_H
#define PROJECT_K_MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MULTIBOOT2_TAG_END          0
#define MULTIBOOT2_TAG_MMAP         6
#define MULTIBOOT2_TAG_FRAMEBUFFER  8

#define MULTIBOOT2_MEMORY_AVAILABLE 1

typedef struct
{
    uint32_t total_size;
    uint32_t reserved;
} multiboot2_info_t;

typedef struct
{
    uint32_t type;
    uint32_t size;
} multiboot2_tag_t;

typedef struct
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} multiboot2_tag_mmap_t;

typedef struct
{
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} multiboot2_mmap_entry_t;

typedef struct
{
    uint32_t type;
    uint32_t size;

    uint64_t framebuffer_addr;

    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;

    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;

    uint8_t color_info[6];
} multiboot2_tag_framebuffer_t;

#endif
