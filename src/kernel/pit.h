#ifndef PROJECT_K_PIT_H
#define PROJECT_K_PIT_H

#include <stdint.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

#define PIT_BASE_FREQUENCY 1193182

void pit_initialize(uint32_t frequency);

#endif
