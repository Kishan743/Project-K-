#include "terminal.h"
#include "framebuffer.h"

static volatile uint16_t* const VGA_MEMORY =
    (uint16_t*)0xB8000;

static uint8_t terminal_row;
static uint8_t terminal_column;
static uint8_t terminal_color;

static inline void outb(
    uint16_t port,
    uint8_t value
)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static uint8_t vga_entry_color(
    vga_color_t foreground,
    vga_color_t background
)
{
    return foreground |
           (background << 4);
}

static uint16_t vga_entry(
    unsigned char character,
    uint8_t color
)
{
    return (uint16_t)character |
           ((uint16_t)color << 8);
}

void terminal_update_cursor(void)
{
    if (framebuffer_is_available())
        return;

    uint16_t position =
        terminal_row * VGA_WIDTH +
        terminal_column;

    outb(0x3D4, 14);
    outb(0x3D5, position >> 8);

    outb(0x3D4, 15);
    outb(0x3D5, position & 0xFF);
}

void terminal_putentryat(
    char c,
    vga_color_t foreground,
    vga_color_t background,
    uint8_t x,
    uint8_t y
)
{
    if (x >= VGA_WIDTH ||
        y >= VGA_HEIGHT)
        return;

    uint8_t color =
        vga_entry_color(
            foreground,
            background
        );

    VGA_MEMORY[
        y * VGA_WIDTH + x
    ] = vga_entry(c, color);
}

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;

    terminal_color =
        vga_entry_color(
            VGA_COLOR_LIGHT_GREY,
            VGA_COLOR_BLACK
        );

    if (framebuffer_is_available())
    {
        framebuffer_setcolor(
            0x00FFFFFF,
            0x00101828
        );

        framebuffer_clear(
            0x00101828
        );

        return;
    }

    terminal_clear();
}

void terminal_setcolor(
    vga_color_t foreground,
    vga_color_t background
)
{
    terminal_color =
        vga_entry_color(
            foreground,
            background
        );

    if (!framebuffer_is_available())
        return;

    uint32_t foreground_rgb =
        0x00FFFFFF;

    uint32_t background_rgb =
        0x00101828;

    switch (foreground)
    {
        case VGA_COLOR_BLACK:
            foreground_rgb = 0x00000000;
            break;

        case VGA_COLOR_RED:
            foreground_rgb = 0x00FF5555;
            break;

        case VGA_COLOR_GREEN:
            foreground_rgb = 0x0055FF55;
            break;

        case VGA_COLOR_BLUE:
            foreground_rgb = 0x005555FF;
            break;

        case VGA_COLOR_CYAN:
            foreground_rgb = 0x0055FFFF;
            break;

        case VGA_COLOR_MAGENTA:
            foreground_rgb = 0x00FF55FF;
            break;

        case VGA_COLOR_BROWN:
            foreground_rgb = 0x00FFFF55;
            break;

        case VGA_COLOR_DARK_GREY:
            foreground_rgb = 0x00777777;
            break;

        case VGA_COLOR_LIGHT_GREY:
            foreground_rgb = 0x00CCCCCC;
            break;

        case VGA_COLOR_LIGHT_RED:
            foreground_rgb = 0x00FF7777;
            break;

        case VGA_COLOR_LIGHT_GREEN:
            foreground_rgb = 0x0077FF77;
            break;

        case VGA_COLOR_LIGHT_BLUE:
            foreground_rgb = 0x007777FF;
            break;

        case VGA_COLOR_LIGHT_CYAN:
            foreground_rgb = 0x0077FFFF;
            break;

        case VGA_COLOR_LIGHT_MAGENTA:
            foreground_rgb = 0x00FF77FF;
            break;

        case VGA_COLOR_LIGHT_BROWN:
            foreground_rgb = 0x00FFFF77;
            break;

        case VGA_COLOR_WHITE:
            foreground_rgb = 0x00FFFFFF;
            break;
    }

    switch (background)
    {
        case VGA_COLOR_BLACK:
            background_rgb = 0x00101828;
            break;

        case VGA_COLOR_RED:
            background_rgb = 0x00200020;
            break;

        case VGA_COLOR_GREEN:
            background_rgb = 0x00002020;
            break;

        case VGA_COLOR_BLUE:
            background_rgb = 0x00002040;
            break;

        case VGA_COLOR_DARK_GREY:
            background_rgb = 0x00202020;
            break;

        default:
            background_rgb = 0x00101828;
            break;
    }

    framebuffer_setcolor(
        foreground_rgb,
        background_rgb
    );
}

void terminal_clear(void)
{
    terminal_row = 0;
    terminal_column = 0;

    if (framebuffer_is_available())
    {
        framebuffer_clear(
            0x00101828
        );

        return;
    }

    for (uint8_t y = 0;
         y < VGA_HEIGHT;
         y++)
    {
        for (uint8_t x = 0;
             x < VGA_WIDTH;
             x++)
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

    terminal_update_cursor();
}

void terminal_putchar(char c)
{
    if (framebuffer_is_available())
    {
        framebuffer_putchar(c);
        return;
    }

    if (c == '\n')
    {
        terminal_column = 0;

        if (terminal_row + 1 >= VGA_HEIGHT)
        {
            for (uint8_t y = 1;
                 y < VGA_HEIGHT;
                 y++)
            {
                for (uint8_t x = 0;
                     x < VGA_WIDTH;
                     x++)
                {
                    VGA_MEMORY[
                        (y - 1) * VGA_WIDTH + x
                    ] =
                        VGA_MEMORY[
                            y * VGA_WIDTH + x
                        ];
                }
            }

            for (uint8_t x = 0;
                 x < VGA_WIDTH;
                 x++)
            {
                terminal_putentryat(
                    ' ',
                    VGA_COLOR_LIGHT_GREY,
                    VGA_COLOR_BLACK,
                    x,
                    VGA_HEIGHT - 1
                );
            }

            terminal_row =
                VGA_HEIGHT - 1;
        }
        else
        {
            terminal_row++;
        }

        terminal_update_cursor();
        return;
    }

    if (c == '\r')
    {
        terminal_column = 0;
        terminal_update_cursor();
        return;
    }

    if (c == '\b')
    {
        if (terminal_column > 0)
        {
            terminal_column--;

            terminal_putentryat(
                ' ',
                VGA_COLOR_LIGHT_GREY,
                VGA_COLOR_BLACK,
                terminal_column,
                terminal_row
            );
        }

        terminal_update_cursor();
        return;
    }

    terminal_putentryat(
        c,
        VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_BLACK,
        terminal_column,
        terminal_row
    );

    terminal_column++;

    if (terminal_column >= VGA_WIDTH)
    {
        terminal_column = 0;

        if (terminal_row + 1 < VGA_HEIGHT)
            terminal_row++;
    }

    terminal_update_cursor();
}

void terminal_write(const char* string)
{
    if (string == 0)
        return;

    while (*string != '\0')
    {
        terminal_putchar(*string);
        string++;
    }
}

void terminal_write_hex(uint32_t value)
{
    const char* digits =
        "0123456789ABCDEF";

    terminal_write("0x");

    for (int shift = 28;
         shift >= 0;
         shift -= 4)
    {
        terminal_putchar(
            digits[(value >> shift) & 0xF]
        );
    }
}
