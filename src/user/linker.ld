ENTRY(_start)

SECTIONS
{
    . = 0x40000000;

    .text : ALIGN(4096)
    {
        *(.text*)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
