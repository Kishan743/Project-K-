#include "console.h"
#include "keyboard.h"
#include "timer.h"
#include "../drivers/terminal.h"

#define CONSOLE_BUFFER_SIZE 128

static char input_buffer[CONSOLE_BUFFER_SIZE];
static unsigned int input_length;

static void console_prompt(void)
{
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("ProjectK> ");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static int string_equals(const char* a, const char* b)
{
    while (*a && *b)
    {
        if (*a != *b)
        {
            return 0;
        }

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

static void console_clear_input(void)
{
    input_length = 0;
}

static void console_backspace(void)
{
    if (input_length == 0)
    {
        return;
    }

    input_length--;
    terminal_putchar('\b');
}

static void console_print_uint(uint32_t value)
{
    char buffer[11];
    int i = 0;

    if (value == 0)
    {
        terminal_putchar('0');
        return;
    }

    while (value > 0)
    {
        buffer[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0)
    {
        terminal_putchar(buffer[--i]);
    }
}

static void console_execute(void)
{
    input_buffer[input_length] = '\0';

    terminal_putchar('\n');

    if (string_equals(input_buffer, "hello"))
    {
        terminal_write("Hello from Project K!\n");
    }
    else if (string_equals(input_buffer, "help"))
    {
        terminal_write("Available commands:\n");
        terminal_write("  hello - Test the shell\n");
        terminal_write("  help  - Show this help message\n");
        terminal_write("  clear - Clear the screen\n");
        terminal_write("  ticks - Show timer ticks\n");
        terminal_write("  info  - Show kernel information\n");
    }
    else if (string_equals(input_buffer, "clear"))
    {
        terminal_clear();
    }
    else if (string_equals(input_buffer, "ticks"))
    {
        terminal_write("Timer ticks: ");
        console_print_uint(timer_get_ticks());
        terminal_putchar('\n');
    }
    else if (string_equals(input_buffer, "info"))
    {
        terminal_write("Project K Kernel\n");
        terminal_write("Architecture: i386\n");
        terminal_write("Interrupts: enabled\n");
        terminal_write("Timer: PIT 100 Hz\n");
        terminal_write("Keyboard: IRQ1\n");
    }
    else if (input_length != 0)
    {
        terminal_write("Unknown command: ");
        terminal_write(input_buffer);
        terminal_write("\n");
    }

    console_clear_input();
    console_prompt();
}

void console_initialize(void)
{
    input_length = 0;
    console_prompt();
}

void console_process_input(void)
{
    while (keyboard_has_input())
    {
        char c = keyboard_getchar();

        if (c == '\b')
        {
            console_backspace();
            continue;
        }

        if (c == '\n')
        {
            console_execute();
            continue;
        }

        if (c >= 32 && c <= 126)
        {
            if (input_length < CONSOLE_BUFFER_SIZE - 1)
            {
                input_buffer[input_length++] = c;
                terminal_putchar(c);
            }
        }
    }
}
