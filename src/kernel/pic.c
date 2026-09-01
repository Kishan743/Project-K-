#include "pic.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void io_wait(void)
{
    outb(0x80, 0);
}

void pic_initialize(void)
{
    uint8_t master_mask = inb(PIC1_DATA);
    uint8_t slave_mask = inb(PIC2_DATA);

    /* Begin PIC initialization */
    outb(PIC1_COMMAND, 0x11);
    io_wait();

    outb(PIC2_COMMAND, 0x11);
    io_wait();

    /* Remap IRQs:
       Master: 32-39
       Slave:  40-47
    */
    outb(PIC1_DATA, 0x20);
    io_wait();

    outb(PIC2_DATA, 0x28);
    io_wait();

    /* Tell master that slave is connected to IRQ2 */
    outb(PIC1_DATA, 0x04);
    io_wait();

    /* Tell slave its cascade identity */
    outb(PIC2_DATA, 0x02);
    io_wait();

    /* 8086/88 mode */
    outb(PIC1_DATA, 0x01);
    io_wait();

    outb(PIC2_DATA, 0x01);
    io_wait();

    /* Restore existing interrupt masks */
    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

void pic_set_mask(uint8_t irq)
{
    if (irq < 8)
    {
        uint8_t mask = inb(PIC1_DATA);
        mask |= (uint8_t)(1 << irq);
        outb(PIC1_DATA, mask);
    }
    else
    {
        irq -= 8;

        uint8_t mask = inb(PIC2_DATA);
        mask |= (uint8_t)(1 << irq);
        outb(PIC2_DATA, mask);
    }
}

void pic_clear_mask(uint8_t irq)
{
    if (irq < 8)
    {
        uint8_t mask = inb(PIC1_DATA);
        mask &= (uint8_t)~(1 << irq);
        outb(PIC1_DATA, mask);
    }
    else
    {
        irq -= 8;

        uint8_t mask = inb(PIC2_DATA);
        mask &= (uint8_t)~(1 << irq);
        outb(PIC2_DATA, mask);
    }
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    outb(PIC1_COMMAND, PIC_EOI);
}
