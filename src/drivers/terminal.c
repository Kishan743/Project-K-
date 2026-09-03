#include "terminal.h"

static volatile uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;

static uint8_t terminal_row;
static uint8_t terminal_column;
static uint8_t terminal_color;

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static uint8_t vga_entry_color(vga_color_t foreground, vga_color_t background)
{
    return foreground | (background << 4);
}

static uint16_t vga_entry(unsigned char character, uint8_t color)
{
    return (uint16_t)character | ((uint16_t)color << 8);
}

void terminal_update_cursor(void)
{
    uint16_t position =
        (uint16_t)(terminal_row * VGA_WIDTH + terminal_column);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

static void terminal_scroll(void)
{
    for (uint8_t y = 1; y < VGA_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < VGA_WIDTH; x++)
        {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] =
                VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }

    for (uint8_t x = 0; x < VGA_WIDTH; x++)
    {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            vga_entry(' ', terminal_color);
    }

    terminal_row = VGA_HEIGHT - 1;
    terminal_column = 0;
}

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;

    terminal_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_clear();
    terminal_update_cursor();
}

void terminal_setcolor(vga_color_t foreground, vga_color_t background)
{
    terminal_color = vga_entry_color(foreground, background);
}

void terminal_putentryat(char c,
                         vga_color_t foreground,
                         vga_color_t background,
                         uint8_t x,
                         uint8_t y)
{
    const uint8_t color = vga_entry_color(foreground, background);
    const uint16_t index = y * VGA_WIDTH + x;

    VGA_MEMORY[index] = vga_entry(c, color);
}

void terminal_clear(void)
{
    for (uint8_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putentryat(
                ' ',
                VGA_COLOR_LIGHT_GREY,
                VGA_COLOR_BLACK,
                x,
                y
            );
        }
    }

    terminal_row = 0;
    terminal_column = 0;

    terminal_update_cursor();
}

void terminal_putchar(char c)
{
    if (c == '\b')
    {
        if (terminal_column > 0)
        {
            terminal_column--;

            terminal_putentryat(
                ' ',
                (vga_color_t)(terminal_color & 0x0F),
                (vga_color_t)((terminal_color >> 4) & 0x0F),
                terminal_column,
                terminal_row
            );
        }

        terminal_update_cursor();
        return;
    }

    if (c == '\n')
    {
        terminal_column = 0;

        if (terminal_row < VGA_HEIGHT - 1)
        {
            terminal_row++;
        }
        else
        {
            terminal_scroll();
        }

        terminal_update_cursor();
        return;
    }

    terminal_putentryat(
        c,
        (vga_color_t)(terminal_color & 0x0F),
        (vga_color_t)((terminal_color >> 4) & 0x0F),
        terminal_column,
        terminal_row
    );

    terminal_column++;

    if (terminal_column == VGA_WIDTH)
    {
        terminal_column = 0;

        if (terminal_row < VGA_HEIGHT - 1)
        {
            terminal_row++;
        }
        else
        {
            terminal_scroll();
        }
    }

    terminal_update_cursor();
}

void terminal_write(const char* string)
{
    while (*string != '\0')
    {
        terminal_putchar(*string);
        string++;
    }
}
