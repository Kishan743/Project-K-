#include "gdt.h"
#include "tss.h"

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

static struct gdt_entry gdt[6];

static struct gdt_ptr gdt_descriptor;

extern void gdt_flush(uint32_t);

static void gdt_set_entry(
    int index,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t granularity
)
{
    gdt[index].base_low =
        base & 0xFFFF;

    gdt[index].base_middle =
        (base >> 16) & 0xFF;

    gdt[index].base_high =
        (base >> 24) & 0xFF;

    gdt[index].limit_low =
        limit & 0xFFFF;

    gdt[index].granularity =
        (limit >> 16) & 0x0F;

    gdt[index].granularity |=
        granularity & 0xF0;

    gdt[index].access = access;
}

void gdt_initialize(void)
{
    gdt_descriptor.limit =
        sizeof(gdt) - 1;

    gdt_descriptor.base =
        (uint32_t)&gdt;

    /*
     * Entry 0:
     * Null descriptor.
     */
    gdt_set_entry(
        0,
        0,
        0,
        0,
        0
    );

    /*
     * Entry 1:
     * Kernel code, Ring 0.
     */
    gdt_set_entry(
        1,
        0,
        0xFFFFFFFF,
        0x9A,
        0xCF
    );

    /*
     * Entry 2:
     * Kernel data, Ring 0.
     */
    gdt_set_entry(
        2,
        0,
        0xFFFFFFFF,
        0x92,
        0xCF
    );

    /*
     * Entry 3:
     * User code, Ring 3.
     *
     * 0xFA:
     * present
     * DPL 3
     * executable
     * readable
     */
    gdt_set_entry(
        3,
        0,
        0xFFFFFFFF,
        0xFA,
        0xCF
    );

    /*
     * Entry 4:
     * User data, Ring 3.
     *
     * 0xF2:
     * present
     * DPL 3
     * writable
     */
    gdt_set_entry(
        4,
        0,
        0xFFFFFFFF,
        0xF2,
        0xCF
    );

    /*
     * Initialize the TSS before installing its descriptor.
     *
     * stack_top is the kernel's boot stack.
     */
    extern uint8_t stack_top;

    tss_initialize(
        (uint32_t)&stack_top
    );

    tss_entry_t* tss =
        tss_get();

    /*
     * Entry 5:
     * 32-bit available TSS.
     *
     * 0x89:
     * present
     * DPL 0
     * available 32-bit TSS
     *
     * 0x40:
     * byte granularity
     */
    gdt_set_entry(
        5,
        (uint32_t)tss,
        sizeof(tss_entry_t) - 1,
        0x89,
        0x40
    );

    /*
     * Load GDT, reload segment registers and load TR.
     */
    gdt_flush(
        (uint32_t)&gdt_descriptor
    );
}
