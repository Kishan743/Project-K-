#ifndef PROJECT_K_FRAMEBUFFER_H
#define PROJECT_K_FRAMEBUFFER_H

#include <stdint.h>

#define FRAMEBUFFER_VIRTUAL_BASE 0xE0000000

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t type;
    uint32_t address;
    uint8_t initialized;
} framebuffer_info_t;

int framebuffer_initialize(uint32_t multiboot_info);

void framebuffer_clear(uint32_t color);

void framebuffer_putpixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
);

void framebuffer_fill_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
);

void framebuffer_putchar(char c);

void framebuffer_write(const char* string);

void framebuffer_setcolor(
    uint32_t foreground,
    uint32_t background
);

uint32_t framebuffer_get_width(void);
uint32_t framebuffer_get_height(void);
uint32_t framebuffer_get_pitch(void);
uint32_t framebuffer_get_bpp(void);

int framebuffer_is_available(void);

#endif
