#ifndef PROJECT_K_TIMER_H
#define PROJECT_K_TIMER_H

#include <stdint.h>

void timer_initialize(uint32_t frequency);
uint32_t timer_get_ticks(void);
void timer_handler(void);
void timer_sleep(uint32_t ticks);

#endif
