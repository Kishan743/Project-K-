#include "drivers/terminal.h"

void kernel_main(void)
{
    terminal_initialize();

    terminal_write("PROJECT K - KERNEL K v0.2\n");
    terminal_write("-------------------------\n");
    terminal_write("VGA terminal driver initialized.\n");
    terminal_write("Kernel boot successful.\n");

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
