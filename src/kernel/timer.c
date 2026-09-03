#include "timer.h"
#include "pit.h"

static volatile uint32_t timer_ticks = 0;

void timer_initialize(uint32_t frequency)
{
    timer_ticks = 0;
    pit_initialize(frequency);
}

void timer_handler(void)
{
    timer_ticks++;
}

uint32_t timer_get_ticks(void)
{
    return timer_ticks;
}

void timer_sleep(uint32_t ticks)
{
    uint32_t start = timer_get_ticks();

    while ((timer_get_ticks() - start) < ticks)
    {
        __asm__ volatile ("hlt");
    }
}
