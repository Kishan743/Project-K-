#include "pit.h"

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

void pit_initialize(uint32_t frequency)
{
    if (frequency == 0)
    {
        return;
    }

    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;

    if (divisor == 0)
    {
        divisor = 1;
    }

    if (divisor > 65535)
    {
        divisor = 65535;
    }

    /*
     * Channel 0
     * Access mode: lobyte/hibyte
     * Operating mode: rate generator
     * Binary mode
     */
    outb(PIT_COMMAND, 0x34);

    /* Send divisor low byte */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));

    /* Send divisor high byte */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}
