#include "cpu.h"

void cpu_enable_interrupts(void)
{
    __asm__ volatile ("sti");
}

void cpu_disable_interrupts(void)
{
    __asm__ volatile ("cli");
}

void cpu_halt(void)
{
    __asm__ volatile ("hlt");
}
