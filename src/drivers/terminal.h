#ifndef PROJECT_K_TERMINAL_H
#define PROJECT_K_TERMINAL_H

#include <stdint.h>

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

typedef enum
{
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
} vga_color_t;

void terminal_initialize(void);
void terminal_clear(void);

void terminal_setcolor(vga_color_t foreground, vga_color_t background);

void terminal_putentryat(char c,
                         vga_color_t foreground,
                         vga_color_t background,
                         uint8_t x,
                         uint8_t y);

void terminal_putchar(char c);
void terminal_write(const char* string);

void terminal_update_cursor(void);

#endif
