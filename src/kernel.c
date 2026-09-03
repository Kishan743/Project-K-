#include "drivers/terminal.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/pic.h"
#include "kernel/timer.h"
#include "kernel/keyboard.h"

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
    keyboard_initialize();

    pic_clear_mask(0);
    pic_clear_mask(1);

    __asm__ volatile ("sti");

    terminal_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("GDT initialized successfully.\n");
    terminal_write("IDT initialized successfully.\n");
    terminal_write("Timer initialized at 100 Hz.\n");
    terminal_write("Keyboard IRQ1 initialized.\n");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
