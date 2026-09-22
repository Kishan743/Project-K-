#ifndef PROJECT_K_PMM_H
#define PROJECT_K_PMM_H

#include <stdint.h>

#define PMM_PAGE_SIZE 4096

void pmm_initialize(uint32_t multiboot_info);

uint32_t pmm_get_total_frames(void);
uint32_t pmm_get_free_frames(void);

void* pmm_alloc_frame(void);
void pmm_free_frame(void* address);

#endif
