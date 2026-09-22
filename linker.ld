ENTRY(start)

SECTIONS
{
    . = 1M;

    __kernel_start = .;

    .text : ALIGN(4K)
    {
        KEEP(*(.multiboot))
        KEEP(*(.multiboot2))
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4K);
    __kernel_end = .;
}
