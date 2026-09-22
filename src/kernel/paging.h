#ifndef PROJECT_K_PAGING_H
#define PROJECT_K_PAGING_H

#include <stdint.h>

#define PAGE_SIZE 4096

#define PAGE_PRESENT  0x001
#define PAGE_WRITABLE 0x002
#define PAGE_USER     0x004

void paging_initialize(void);

int paging_map_page(uint32_t virtual_address,
                    uint32_t physical_address,
                    uint32_t flags);

int paging_unmap_page(uint32_t virtual_address);

uint32_t paging_get_directory(void);

#endif
