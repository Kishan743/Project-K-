#ifndef PROJECT_K_CPU_H
#define PROJECT_K_CPU_H

#include <stdint.h>

typedef struct cpu_context
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t saved_esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t interrupt_number;
    uint32_t error_code;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

    uint32_t useresp;
    uint32_t userss;

} cpu_context_t;

void cpu_enable_interrupts(void);
void cpu_disable_interrupts(void);
void cpu_halt(void);

#endif
