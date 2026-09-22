#include "framebuffer.h"

#include "kernel/multiboot.h"
#include "kernel/paging.h"

static framebuffer_info_t framebuffer;

static uint32_t cursor_x;
static uint32_t cursor_y;

static uint32_t foreground_color = 0x00FFFFFF;
static uint32_t background_color = 0x00101828;

#define FONT_WIDTH  5
#define FONT_HEIGHT 7
#define CHAR_WIDTH  6
#define CHAR_HEIGHT 9

/*
 * Compact 5x7 font.
 *
 * Each character is represented by five columns.
 * Bit 0 is the top pixel.
 */

static const uint8_t font_digits[10][5] =
{
    {0x3E,0x51,0x49,0x45,0x3E},
    {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},
    {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E}
};

static const uint8_t font_upper[26][5] =
{
    {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},
    {0x7F,0x20,0x18,0x20,0x7F},
    {0x63,0x14,0x08,0x14,0x63},
    {0x03,0x04,0x78,0x04,0x03},
    {0x61,0x51,0x49,0x45,0x43}
};

static uint8_t lowercase_column(
    char c,
    uint32_t column
)
{
    /*
     * Lowercase letters use the uppercase glyph
     * as a compact fallback. This keeps the kernel
     * font small while preserving readability.
     */
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');

    if (c >= 'A' && c <= 'Z')
        return font_upper[c - 'A'][column];

    return 0;
}

static uint32_t align_down(uint32_t value)
{
    return value & ~(PAGE_SIZE - 1);
}

static uint32_t align_up(uint32_t value)
{
    return (value + PAGE_SIZE - 1) &
           ~(PAGE_SIZE - 1);
}

static multiboot2_tag_framebuffer_t*
find_framebuffer_tag(uint32_t info_address)
{
    multiboot2_info_t* info =
        (multiboot2_info_t*)info_address;

    uint8_t* current =
        (uint8_t*)info + 8;

    uint8_t* end =
        (uint8_t*)info + info->total_size;

    while (current < end)
    {
        multiboot2_tag_t* tag =
            (multiboot2_tag_t*)current;

        if (tag->type ==
            MULTIBOOT2_TAG_FRAMEBUFFER)
        {
            return
                (multiboot2_tag_framebuffer_t*)tag;
        }

        if (tag->type ==
            MULTIBOOT2_TAG_END)
        {
            break;
        }

        if (tag->size < 8)
            break;

        current +=
            (tag->size + 7) & ~7u;
    }

    return 0;
}

int framebuffer_initialize(uint32_t multiboot_info)
{
    multiboot2_tag_framebuffer_t* tag =
        find_framebuffer_tag(multiboot_info);

    if (tag == 0)
        return -1;

    if ((uint32_t)(tag->framebuffer_addr >> 32) != 0)
        return -1;

    if (tag->framebuffer_type != 1)
        return -1;

    if (tag->framebuffer_bpp != 32)
        return -1;

    uint32_t physical =
        (uint32_t)tag->framebuffer_addr;

    uint32_t size =
        tag->framebuffer_pitch *
        tag->framebuffer_height;

    uint32_t physical_base =
        align_down(physical);

    uint32_t offset =
        physical - physical_base;

    uint32_t mapped_size =
        align_up(size + offset);

    uint32_t pages =
        mapped_size / PAGE_SIZE;

    if (pages == 0)
        return -1;

    for (uint32_t i = 0;
         i < pages;
         i++)
    {
        uint32_t physical_page =
            physical_base +
            i * PAGE_SIZE;

        uint32_t virtual_page =
            FRAMEBUFFER_VIRTUAL_BASE +
            i * PAGE_SIZE;

        if (paging_map_page(
                virtual_page,
                physical_page,
                PAGE_PRESENT |
                PAGE_WRITABLE) != 0)
        {
            return -1;
        }
    }

    framebuffer.width =
        tag->framebuffer_width;

    framebuffer.height =
        tag->framebuffer_height;

    framebuffer.pitch =
        tag->framebuffer_pitch;

    framebuffer.bpp =
        tag->framebuffer_bpp;

    framebuffer.type =
        tag->framebuffer_type;

    framebuffer.address =
        FRAMEBUFFER_VIRTUAL_BASE +
        offset;

    framebuffer.initialized = 1;

    cursor_x = 0;
    cursor_y = 0;

    return 0;
}

void framebuffer_clear(uint32_t color)
{
    if (!framebuffer.initialized)
        return;

    framebuffer_fill_rect(
        0,
        0,
        framebuffer.width,
        framebuffer.height,
        color
    );

    cursor_x = 0;
    cursor_y = 0;
}

void framebuffer_putpixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
)
{
    if (!framebuffer.initialized)
        return;

    if (x >= framebuffer.width ||
        y >= framebuffer.height)
        return;

    uint32_t offset =
        y * framebuffer.pitch +
        x * 4;

    *(volatile uint32_t*)
        ((uint8_t*)framebuffer.address + offset) =
        color;
}

void framebuffer_fill_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
)
{
    if (!framebuffer.initialized)
        return;

    if (x >= framebuffer.width ||
        y >= framebuffer.height)
        return;

    if (y + height > framebuffer.height)
        height = framebuffer.height - y;

    if (x + width > framebuffer.width)
        width = framebuffer.width - x;

    for (uint32_t py = 0;
         py < height;
         py++)
    {
        volatile uint32_t* row =
            (volatile uint32_t*)
            ((uint8_t*)framebuffer.address +
             (y + py) * framebuffer.pitch +
             x * 4);

        for (uint32_t px = 0;
             px < width;
             px++)
        {
            row[px] = color;
        }
    }
}

static void scroll(void)
{
    if (!framebuffer.initialized)
        return;

    uint32_t line_height = CHAR_HEIGHT;

    uint32_t source_y = line_height;
    uint32_t destination_y = 0;

    for (uint32_t y = source_y;
         y < framebuffer.height;
         y++)
    {
        volatile uint32_t* destination =
            (volatile uint32_t*)
            ((uint8_t*)framebuffer.address +
             destination_y * framebuffer.pitch);

        volatile uint32_t* source =
            (volatile uint32_t*)
            ((uint8_t*)framebuffer.address +
             y * framebuffer.pitch);

        uint32_t pixels =
            framebuffer.width;

        for (uint32_t x = 0;
             x < pixels;
             x++)
        {
            destination[x] = source[x];
        }

        destination_y++;
    }

    framebuffer_fill_rect(
        0,
        framebuffer.height - line_height,
        framebuffer.width,
        line_height,
        background_color
    );

    if (cursor_y >= line_height)
        cursor_y -= line_height;
    else
        cursor_y = 0;
}

static void newline(void)
{
    cursor_x = 0;
    cursor_y += CHAR_HEIGHT;

    if (cursor_y + CHAR_HEIGHT >
        framebuffer.height)
    {
        scroll();
    }
}

static void draw_glyph(
    char c,
    uint32_t x,
    uint32_t y
)
{
    uint8_t glyph[5] = {0,0,0,0,0};

    if (c >= '0' && c <= '9')
    {
        for (uint32_t i = 0; i < 5; i++)
            glyph[i] = font_digits[c - '0'][i];
    }
    else if (c >= 'A' && c <= 'Z')
    {
        for (uint32_t i = 0; i < 5; i++)
            glyph[i] = font_upper[c - 'A'][i];
    }
    else if (c >= 'a' && c <= 'z')
    {
        for (uint32_t i = 0; i < 5; i++)
            glyph[i] = lowercase_column(c, i);
    }
    else
    {
        switch (c)
        {
            case ' ':
                return;

            case '.':
                glyph[0] = 0x00;
                glyph[1] = 0x00;
                glyph[2] = 0x40;
                glyph[3] = 0x00;
                glyph[4] = 0x00;
                break;

            case ',':
                glyph[0] = 0x00;
                glyph[1] = 0x00;
                glyph[2] = 0x40;
                glyph[3] = 0x20;
                glyph[4] = 0x00;
                break;

            case ':':
                glyph[0] = 0x00;
                glyph[1] = 0x24;
                glyph[2] = 0x00;
                glyph[3] = 0x24;
                glyph[4] = 0x00;
                break;

            case '-':
                glyph[0] = 0x08;
                glyph[1] = 0x08;
                glyph[2] = 0x08;
                glyph[3] = 0x08;
                glyph[4] = 0x08;
                break;

            case '_':
                glyph[0] = 0x40;
                glyph[1] = 0x40;
                glyph[2] = 0x40;
                glyph[3] = 0x40;
                glyph[4] = 0x40;
                break;

            case '/':
                glyph[0] = 0x60;
                glyph[1] = 0x10;
                glyph[2] = 0x08;
                glyph[3] = 0x04;
                glyph[4] = 0x03;
                break;

            case '\\':
                glyph[0] = 0x03;
                glyph[1] = 0x04;
                glyph[2] = 0x08;
                glyph[3] = 0x10;
                glyph[4] = 0x60;
                break;

            case '>':
                glyph[0] = 0x08;
                glyph[1] = 0x14;
                glyph[2] = 0x22;
                glyph[3] = 0x41;
                glyph[4] = 0x00;
                break;

            case '<':
                glyph[0] = 0x00;
                glyph[1] = 0x41;
                glyph[2] = 0x22;
                glyph[3] = 0x14;
                glyph[4] = 0x08;
                break;

            case '!':
                glyph[0] = 0x00;
                glyph[1] = 0x00;
                glyph[2] = 0x5F;
                glyph[3] = 0x00;
                glyph[4] = 0x00;
                break;

            case '?':
                glyph[0] = 0x02;
                glyph[1] = 0x01;
                glyph[2] = 0x51;
                glyph[3] = 0x09;
                glyph[4] = 0x06;
                break;

            case '(':
                glyph[0] = 0x00;
                glyph[1] = 0x1C;
                glyph[2] = 0x22;
                glyph[3] = 0x41;
                glyph[4] = 0x00;
                break;

            case ')':
                glyph[0] = 0x00;
                glyph[1] = 0x41;
                glyph[2] = 0x22;
                glyph[3] = 0x1C;
                glyph[4] = 0x00;
                break;

            default:
                glyph[0] = 0x7F;
                glyph[1] = 0x41;
                glyph[2] = 0x41;
                glyph[3] = 0x41;
                glyph[4] = 0x7F;
                break;
        }
    }

    for (uint32_t column = 0;
         column < FONT_WIDTH;
         column++)
    {
        uint8_t bits = glyph[column];

        for (uint32_t row = 0;
             row < FONT_HEIGHT;
             row++)
        {
            uint32_t color =
                (bits & (1u << row))
                ? foreground_color
                : background_color;

            framebuffer_putpixel(
                x + column,
                y + row,
                color
            );
        }
    }
}

void framebuffer_putchar(char c)
{
    if (!framebuffer.initialized)
        return;

    if (c == '\n')
    {
        newline();
        return;
    }

    if (c == '\r')
    {
        cursor_x = 0;
        return;
    }

    if (c == '\b')
    {
        if (cursor_x >= CHAR_WIDTH)
            cursor_x -= CHAR_WIDTH;

        framebuffer_fill_rect(
            cursor_x,
            cursor_y,
            CHAR_WIDTH,
            CHAR_HEIGHT,
            background_color
        );

        return;
    }

    if (cursor_x + CHAR_WIDTH >
        framebuffer.width)
    {
        newline();
    }

    draw_glyph(
        c,
        cursor_x,
        cursor_y
    );

    cursor_x += CHAR_WIDTH;
}

void framebuffer_write(const char* string)
{
    if (string == 0)
        return;

    while (*string != '\0')
    {
        framebuffer_putchar(*string);
        string++;
    }
}

void framebuffer_setcolor(
    uint32_t foreground,
    uint32_t background
)
{
    foreground_color = foreground;
    background_color = background;
}

uint32_t framebuffer_get_width(void)
{
    return framebuffer.width;
}

uint32_t framebuffer_get_height(void)
{
    return framebuffer.height;
}

uint32_t framebuffer_get_pitch(void)
{
    return framebuffer.pitch;
}

uint32_t framebuffer_get_bpp(void)
{
    return framebuffer.bpp;
}

int framebuffer_is_available(void)
{
    return framebuffer.initialized != 0;
}
