#include "tss.h"
#include "gdt.h"

static tss_entry_t tss;

void tss_initialize(uint32_t kernel_stack)
{
    /*
     * Start with a completely clean TSS.
     */
    for (uint32_t i = 0;
         i < sizeof(tss_entry_t) / sizeof(uint32_t);
         i++)
    {
        ((uint32_t*)&tss)[i] = 0;
    }

    /*
     * When the CPU transitions from Ring 3 to Ring 0,
     * it loads ESP from ESP0 and SS from SS0.
     */
    tss.esp0 = kernel_stack;
    tss.ss0 = GDT_KERNEL_DATA;

    /*
     * The TSS itself is a 32-bit available TSS.
     */
    tss.iomap_base = sizeof(tss_entry_t);
}

tss_entry_t* tss_get(void)
{
    return &tss;
}
