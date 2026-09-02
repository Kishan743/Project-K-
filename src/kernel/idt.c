#include "idt.h"
#include "../drivers/terminal.h"
#include "timer.h"
#include "pic.h"
extern void idt_flush(uint32_t);

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

extern void pic_send_eoi(uint8_t irq);

struct idt_entry
{
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

static struct idt_entry idt[256];

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_ptr idt_descriptor;

static void idt_set_gate(
    uint8_t index,
    uint32_t base,
    uint16_t selector,
    uint8_t flags
)
{
    idt[index].base_low = base & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].flags = flags;
    idt[index].base_high = (base >> 16) & 0xFFFF;
}

void exception_handler(struct interrupt_frame* frame)
{
    terminal_write("CPU EXCEPTION: ");

    if (frame->interrupt_number == 0)
    {
        terminal_write("DIVIDE ERROR\n");
    }
    else if (frame->interrupt_number == 1)
    {
        terminal_write("DEBUG EXCEPTION\n");
    }
    else if (frame->interrupt_number == 2)
    {
        terminal_write("NON-MASKABLE INTERRUPT\n");
    }
    else if (frame->interrupt_number == 3)
    {
        terminal_write("BREAKPOINT EXCEPTION\n");
    }
    else if (frame->interrupt_number == 4)
    {
        terminal_write("OVERFLOW EXCEPTION\n");
    }
    else if (frame->interrupt_number == 5)
    {
        terminal_write("BOUND RANGE EXCEEDED\n");
    }
    else if (frame->interrupt_number == 6)
    {
        terminal_write("INVALID OPCODE\n");
    }
    else if (frame->interrupt_number == 7)
    {
        terminal_write("DEVICE NOT AVAILABLE\n");
    }
    else if (frame->interrupt_number == 8)
    {
        terminal_write("DOUBLE FAULT\n");
    }
    else if (frame->interrupt_number == 9)
    {
        terminal_write("COPROCESSOR SEGMENT OVERRUN\n");
    }
    else if (frame->interrupt_number == 10)
    {
        terminal_write("INVALID TSS\n");
    }
    else if (frame->interrupt_number == 11)
    {
        terminal_write("SEGMENT NOT PRESENT\n");
    }
    else if (frame->interrupt_number == 12)
    {
        terminal_write("STACK-SEGMENT FAULT\n");
    }
    else if (frame->interrupt_number == 13)
    {
        terminal_write("GENERAL PROTECTION FAULT\n");
    }
    else if (frame->interrupt_number == 14)
    {
        terminal_write("PAGE FAULT\n");
    }
    else if (frame->interrupt_number == 16)
    {
        terminal_write("x87 FLOATING-POINT EXCEPTION\n");
    }
    else if (frame->interrupt_number == 17)
    {
        terminal_write("ALIGNMENT CHECK\n");
    }
    else if (frame->interrupt_number == 18)
    {
        terminal_write("MACHINE CHECK\n");
    }
    else if (frame->interrupt_number == 19)
    {
        terminal_write("SIMD FLOATING-POINT EXCEPTION\n");
    }
    else
    {
        terminal_write("RESERVED/UNKNOWN EXCEPTION\n");
    }

    while (1)
    {
        __asm__ volatile ("cli; hlt");
}
}
void irq_handler(struct interrupt_frame* frame)
{
    uint32_t irq = frame->interrupt_number - 32;

    if (irq == 0)
    {
        timer_handler();
    }

    pic_send_eoi((uint8_t)irq);
}
void idt_initialize(void)
{
    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base = (uint32_t)&idt;

    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);


 /* Hardware IRQs: vectors 32-47 */
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);

    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

    idt_flush((uint32_t)&idt_descriptor);
}
