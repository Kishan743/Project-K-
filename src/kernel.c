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

    terminal_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("Kernel boot successful.\n");

    uint32_t last_ticks = 0;

    while (1)
    {
        __asm__ volatile ("hlt");

        uint32_t current_ticks = timer_get_ticks();

        if (current_ticks != last_ticks)
        {
            last_ticks = current_ticks;
            terminal_write("TICK\n");
        }
    }
}
