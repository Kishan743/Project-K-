#include "terminal.h"

static volatile uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;

static uint8_t terminal_row;
static uint8_t terminal_column;
static uint8_t terminal_color;

static uint8_t vga_entry_color(vga_color_t foreground, vga_color_t background)
{
    return foreground | background << 4;
}

static uint16_t vga_entry(unsigned char character, uint8_t color)
{
    return (uint16_t)character | (uint16_t)color << 8;
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
}

void terminal_putchar(char c)
{
    /*
     * Backspace:
     * Move one position backwards and erase the character.
     */
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

        return;
    }

    /*
     * Newline:
     * Move to the beginning of the next row.
     */
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

        return;
    }

    /*
     * Normal character.
     */
    terminal_putentryat(
        c,
        (vga_color_t)(terminal_color & 0x0F),
        (vga_color_t)((terminal_color >> 4) & 0x0F),
        terminal_column,
        terminal_row
    );

    terminal_column++;

    /*
     * End of line.
     */
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
}

void terminal_write(const char* string)
{
    while (*string != '\0')
    {
        terminal_putchar(*string);
        string++;
    }
}
