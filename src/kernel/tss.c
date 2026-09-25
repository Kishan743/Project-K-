#include "tss.h"
#include "gdt.h"

static tss_entry_t tss;

void tss_initialize(uint32_t kernel_stack)
{
    for (uint32_t i = 0;
         i < sizeof(tss_entry_t) / sizeof(uint32_t);
         i++)
    {
        ((uint32_t*)&tss)[i] = 0;
    }

    tss.esp0 = kernel_stack;
    tss.ss0 = GDT_KERNEL_DATA;
    tss.iomap_base = sizeof(tss_entry_t);
}

void tss_set_kernel_stack(uint32_t kernel_stack)
{
    tss.esp0 = kernel_stack;
}

tss_entry_t* tss_get(void)
{
    return &tss;
}
