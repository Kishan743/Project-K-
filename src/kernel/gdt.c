#include "gdt.h"

struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;

    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr gdt_descriptor;

static void gdt_set_entry(
    int index,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t granularity
)
{
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;

    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].granularity = (limit >> 16) & 0x0F;

    gdt[index].granularity |= granularity & 0xF0;
    gdt[index].access = access;
}

extern void gdt_flush(uint32_t);

void gdt_initialize(void)
{
    gdt_descriptor.limit = sizeof(gdt) - 1;
    gdt_descriptor.base = (uint32_t)&gdt;

    /*
     * Entry 0: Null descriptor.
     */
    gdt_set_entry(
        0,
        0,
        0,
        0,
        0
    );

    /*
     * Entry 1: Kernel code segment.
     * Base = 0
     * Limit = 4 GiB
     * Access = present + ring 0 + executable + readable
     * Granularity = 4 KiB pages + 32-bit protected mode
     */
    gdt_set_entry(
        1,
        0,
        0xFFFFFFFF,
        0x9A,
        0xCF
    );

    /*
     * Entry 2: Kernel data segment.
     * Base = 0
     * Limit = 4 GiB
     * Access = present + ring 0 + writable
     * Granularity = 4 KiB pages + 32-bit protected mode
     */
    gdt_set_entry(
        2,
        0,
        0xFFFFFFFF,
        0x92,
        0xCF
    );

    gdt_flush((uint32_t)&gdt_descriptor);
}
