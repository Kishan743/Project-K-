#include "console.h"
#include "keyboard.h"
#include "timer.h"
#include "pmm.h"
#include "heap.h"
#include "task.h"
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
            return 0;

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
        return;

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
        terminal_putchar(buffer[--i]);
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
        terminal_write("  memory - Show physical memory status\n");
        terminal_write("  alloc - Allocate one physical frame\n");
        terminal_write("  malloc - Allocate kernel heap memory\n");
        terminal_write("  heap - Show kernel heap status\n");
        terminal_write("  tasks - Show scheduler task status\n");
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
    else if (string_equals(input_buffer, "heap"))
    {
        terminal_write("Kernel Heap\n");

        terminal_write("Used: ");
        console_print_uint(heap_get_used());
        terminal_write(" bytes\n");

        terminal_write("Free: ");
        console_print_uint(heap_get_free());
        terminal_write(" bytes\n");
    }
    else if (string_equals(input_buffer, "tasks"))
    {
        const task_t* tasks =
            task_get_table();

        terminal_write("Task Scheduler\n");

        for (uint32_t i = 0;
             i < TASK_MAX;
             i++)
        {
            if (tasks[i].state == TASK_UNUSED)
                continue;

            terminal_write("Task ");
            console_print_uint(tasks[i].id);

            terminal_write(" - ");

            if (tasks[i].state == TASK_READY)
                terminal_write("READY");
            else if (tasks[i].state == TASK_RUNNING)
                terminal_write("RUNNING");
            else if (tasks[i].state == TASK_TERMINATED)
                terminal_write("TERMINATED");
            else
                terminal_write("UNKNOWN");

            terminal_write(" | switches: ");
            console_print_uint(tasks[i].switches);

            terminal_write(" | work: ");
            console_print_uint(tasks[i].work_counter);

            terminal_putchar('\n');
        }
    }
    else if (string_equals(input_buffer, "malloc"))
    {
        void* memory = kmalloc(64);

        if (memory != 0)
        {
            terminal_write("kmalloc(64) successful\n");
        }
        else
        {
            terminal_write("kmalloc failed\n");
        }
    }
    else if (string_equals(input_buffer, "info"))
    {
        terminal_write("Project K Kernel\n");
        terminal_write("Architecture: i386\n");
        terminal_write("Interrupts: enabled\n");
        terminal_write("Timer: PIT 100 Hz\n");
        terminal_write("Keyboard: IRQ1\n");
    }
    else if (string_equals(input_buffer, "memory"))
    {
        terminal_write("Physical Memory Manager\n");

        terminal_write("Total frames: ");
        console_print_uint(pmm_get_total_frames());
        terminal_putchar('\n');

        terminal_write("Free frames: ");
        console_print_uint(pmm_get_free_frames());
        terminal_putchar('\n');
    }
    else if (string_equals(input_buffer, "alloc"))
    {
        void* frame = pmm_alloc_frame();

        if (frame != 0)
        {
            terminal_write("Allocated frame at address 0x");
            
            uint32_t address = (uint32_t)frame;
            char hex[9];
            const char* digits = "0123456789ABCDEF";

            for (int i = 7; i >= 0; i--)
            {
                hex[i] = digits[address & 0xF];
                address >>= 4;
            }

            for (int i = 0; i < 8; i++)
                terminal_putchar(hex[i]);

            terminal_putchar('\n');
        }
        else
        {
            terminal_write("No free physical frames.\n");
        }
    }
    else if (input_length != 0)
    {
        terminal_write("Unknown command: ");
        terminal_write(input_buffer);
        terminal_putchar('\n');
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
