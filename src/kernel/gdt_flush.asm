BITS 32

section .text

global gdt_flush

gdt_flush:
    mov eax, [esp + 4]

    lgdt [eax]

    ; Reload kernel data segments.
    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload kernel code segment.
    jmp 0x08:.flush_code

.flush_code:

    ; Load Task Register with TSS selector 0x28.
    mov ax, 0x28
    ltr ax

    ret

section .note.GNU-stack noalloc noexec nowrite progbits
