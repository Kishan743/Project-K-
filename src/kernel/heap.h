#ifndef PROJECT_K_HEAP_H
#define PROJECT_K_HEAP_H

#include <stdint.h>

void heap_initialize(void);

void* kmalloc(uint32_t size);
void kfree(void* pointer);

uint32_t heap_get_used(void);
uint32_t heap_get_free(void);

#endif
