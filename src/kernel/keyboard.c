#include "keyboard.h"

#define KEYBOARD_DATA_PORT 0x60

#define SCANCODE_SHIFT_LEFT   0x2A
#define SCANCODE_SHIFT_RIGHT  0x36
#define SCANCODE_CAPS_LOCK    0x3A
#define SCANCODE_BACKSPACE    0x0E
#define SCANCODE_ENTER        0x1C

static volatile char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint8_t buffer_head;
static volatile uint8_t buffer_tail;

static uint8_t shift_pressed;
static uint8_t caps_lock;

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
    0, 27,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=', '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p',
    '[',']','\n',
    0,
    'a','s','d','f','g','h','j','k','l',
    ';','\'','`',
    0,
    '\\',
    'z','x','c','v','b','n','m',
    ',','.','/',
    0,
    '*',
    0,
    ' '
};

static const char keyboard_shift_map[128] =
{
    0, 27,
    '!','@','#','$','%','^','&','*','(',')',
    '_','+', '\b',
    '\t',
    'Q','W','E','R','T','Y','U','I','O','P',
    '{','}','\n',
    0,
    'A','S','D','F','G','H','J','K','L',
    ':','"','~',
    0,
    '|',
    'Z','X','C','V','B','N','M',
    '<','>','?',
    0,
    '*',
    0,
    ' '
};

void keyboard_initialize(void)
{
    buffer_head = 0;
    buffer_tail = 0;

    shift_pressed = 0;
    caps_lock = 0;
}

static void keyboard_buffer_push(char c)
{
    uint8_t next_head =
        (uint8_t)((buffer_head + 1) % KEYBOARD_BUFFER_SIZE);

    if (next_head == buffer_tail)
    {
        /* Buffer full: discard the new character. */
        return;
    }

    keyboard_buffer[buffer_head] = c;
    buffer_head = next_head;
}

int keyboard_has_input(void)
{
    return buffer_head != buffer_tail;
}

char keyboard_getchar(void)
{
    char c;

    if (!keyboard_has_input())
    {
        return 0;
    }

    c = keyboard_buffer[buffer_tail];

    buffer_tail =
        (uint8_t)((buffer_tail + 1) % KEYBOARD_BUFFER_SIZE);

    return c;
}

void keyboard_handler(void)
{
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* Key release */
    if (scancode & 0x80)
    {
        uint8_t released = scancode & 0x7F;

        if (released == SCANCODE_SHIFT_LEFT ||
            released == SCANCODE_SHIFT_RIGHT)
        {
            shift_pressed = 0;
        }

        return;
    }

    /* Shift */
    if (scancode == SCANCODE_SHIFT_LEFT ||
        scancode == SCANCODE_SHIFT_RIGHT)
    {
        shift_pressed = 1;
        return;
    }

    /* Caps Lock */
    if (scancode == SCANCODE_CAPS_LOCK)
    {
        caps_lock = !caps_lock;
        return;
    }

    if (scancode >= 128)
    {
        return;
    }

    char c;

    if (shift_pressed)
    {
        c = keyboard_shift_map[scancode];
    }
    else
    {
        c = keyboard_map[scancode];
    }

    /*
     * Caps Lock changes alphabetic characters.
     */
    if (caps_lock && c >= 'a' && c <= 'z')
    {
        c = (char)(c - 'a' + 'A');
    }
    else if (caps_lock && !shift_pressed &&
             c >= 'A' && c <= 'Z')
    {
        c = (char)(c - 'A' + 'a');
    }

    if (c != 0)
    {
        keyboard_buffer_push(c);
    }
}
