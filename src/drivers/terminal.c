#include "terminal.h"

static volatile uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;

static uint8_t terminal_row;
static uint8_t terminal_column;
static uint8_t terminal_color;

static uint16_t vga_entry(unsigned char character, uint8_t color)
{
    return (uint16_t)character | (uint16_t)color << 8;
}

static uint8_t vga_color(uint8_t foreground, uint8_t background)
{
    return foreground | background << 4;
}

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;

    terminal_color = vga_color(7, 0);

    terminal_clear();
}

void terminal_clear(void)
{
    for (uint8_t row = 0; row < VGA_HEIGHT; row++)
    {
        for (uint8_t column = 0; column < VGA_WIDTH; column++)
        {
            const uint8_t index = row * VGA_WIDTH + column;
            VGA_MEMORY[index] = vga_entry(' ', terminal_color);
        }
    }

    terminal_row = 0;
    terminal_column = 0;
}

void terminal_putchar(char c)
{
    if (c == '\n')
    {
        terminal_column = 0;

        if (++terminal_row == VGA_HEIGHT)
        {
            terminal_row = 0;
        }

        return;
    }

    const uint8_t index =
        terminal_row * VGA_WIDTH + terminal_column;

    VGA_MEMORY[index] = vga_entry(c, terminal_color);

    if (++terminal_column == VGA_WIDTH)
    {
        terminal_column = 0;

        if (++terminal_row == VGA_HEIGHT)
        {
            terminal_row = 0;
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
