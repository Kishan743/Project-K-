#ifndef PROJECT_K_GDT_H
#define PROJECT_K_GDT_H

#include <stdint.h>

/*
 * Segment selectors.
 */
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10

#define GDT_USER_CODE   0x1B
#define GDT_USER_DATA   0x23

#define GDT_TSS         0x28

void gdt_initialize(void);

#endif
