#include "drivers/terminal.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/pic.h"
#include "kernel/timer.h"

void kernel_main(void)
{
    terminal_initialize();

    terminal_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("PROJECT K - KERNEL K v0.4\n");

    terminal_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("-------------------------\n");

    terminal_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("VGA terminal driver initialized.\n");

    gdt_initialize();

    pic_initialize();
    idt_initialize();

    timer_initialize(100);
    pic_clear_mask(0);

    __asm__ volatile ("sti");

    terminal_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("GDT initialized successfully.\n");
    terminal_write("IDT initialized successfully.\n");
    terminal_write("Timer initialized at 100 Hz.\n");

    terminal_write("Sleeping for 1 second...\n");
    timer_sleep(100);
    terminal_write("1 second elapsed.\n");

    terminal_write("Sleeping for 2 seconds...\n");
    timer_sleep(200);
    terminal_write("2 seconds elapsed.\n");

    terminal_write("Timer sleep test successful.\n");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
