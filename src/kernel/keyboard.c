#include "keyboard.h"
#include "../drivers/terminal.h"

#define KEYBOARD_DATA_PORT 0x60

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static const char keyboard_map[128] =
{
    0,
    27,
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=', '\b',
    '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']', '\n',
    0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',
    0,
    '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm',
    ',', '.', '/',
    0,
    '*',
    0,
    ' ',
};

void keyboard_initialize(void)
{
    /* PS/2 keyboard is already initialized by firmware. */
}

void keyboard_handler(void)
{
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* Ignore key-release scancodes. */
    if (scancode & 0x80)
    {
        return;
    }

    if (scancode < 128)
    {
        char c = keyboard_map[scancode];

        if (c != 0)
        {
            terminal_putchar(c);
        }
    }
}
