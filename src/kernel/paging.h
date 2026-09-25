#ifndef PROJECT_K_PAGING_H
#define PROJECT_K_PAGING_H

#include <stdint.h>

#define PAGE_SIZE 4096

#define PAGE_PRESENT  0x001
#define PAGE_WRITABLE 0x002
#define PAGE_USER     0x004

typedef struct address_space
{
    uint32_t page_directory;
} address_space_t;

void paging_initialize(void);

int paging_map_page(uint32_t virtual_address,
                    uint32_t physical_address,
                    uint32_t flags);

int paging_unmap_page(uint32_t virtual_address);

uint32_t paging_get_directory(void);

address_space_t* paging_get_kernel_address_space(void);

address_space_t* paging_create_address_space(void);

int paging_map_user_page(address_space_t* space,
                         uint32_t virtual_address,
                         uint32_t physical_address,
                         uint32_t flags);

void paging_switch_address_space(address_space_t* space);

void paging_destroy_address_space(address_space_t* space);

#endif