#ifndef PROJECT_K_IDT_H
#define PROJECT_K_IDT_H

#include <stdint.h>

struct interrupt_frame
{
    uint32_t interrupt_number;
    uint32_t error_code;
};

void idt_initialize(void);

void exception_handler(struct interrupt_frame* frame);
void irq_handler(struct interrupt_frame* frame);

extern volatile uint32_t timer_ticks;

#endif
