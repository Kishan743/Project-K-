BITS 32

section .multiboot2
align 8

MULTIBOOT2_MAGIC          equ 0xE85250D6
MULTIBOOT2_ARCH           equ 0
MULTIBOOT2_HEADER_LENGTH  equ multiboot2_header_end - multiboot2_header
MULTIBOOT2_CHECKSUM       equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_HEADER_LENGTH)

multiboot2_header:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_HEADER_LENGTH
    dd MULTIBOOT2_CHECKSUM

    ; Framebuffer request tag
    dw 5
    dw 0
    dd 20
    dd 0
    dd 0
    dd 0

    ; Padding to 8-byte alignment
    dd 0

    ; End tag
    dw 0
    dw 0
    dd 8

multiboot2_header_end:

section .text
global start
extern kernel_main

start:
    cli

    ; Direct VGA diagnostic: write "K" at top-left.
    mov dword [0xB8000], 0x1F4B1F4B

    ; Verify Multiboot 2 magic in EAX.
    cmp eax, 0x36D76289
    jne .bad_magic

    ; Write "M" if Multiboot 2 reached us.
    mov word [0xB8000], 0x1F4D

    mov esp, stack_top

    ; kernel_main(multiboot_magic, multiboot_info)
    push ebx
    push eax
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

.bad_magic:
    mov word [0xB8000], 0x4F58

.bad_magic_hang:
    cli
    hlt
    jmp .bad_magic_hang

section .bss
align 16

stack_bottom:
    resb 16384
stack_top:

section .note.GNU-stack noalloc noexec nowrite progbits
