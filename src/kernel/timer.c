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

