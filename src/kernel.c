#include "drivers/terminal.h"

void kernel_main(void)
{
    terminal_initialize();

    terminal_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("PROJECT K - KERNEL K v0.3\n");

    terminal_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("-------------------------\n");

    terminal_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("VGA terminal driver initialized.\n");

    terminal_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_write("Color support: ONLINE\n");

    terminal_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_write("Scrolling support: ONLINE\n");
    terminal_write("Kernel boot successful.\n");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
