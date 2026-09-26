BITS 32

GLOBAL _start

SECTION .text

_start:
    ; SYS_WRITE_CHAR('U')
    mov eax, 1
    mov ebx, 'U'
    int 0x80

    ; SYS_GETPID()
    mov eax, 2
    int 0x80

    ; Convert PID to one ASCII digit.
    add eax, '0'
    mov ebx, eax

    ; SYS_WRITE_CHAR(pid)
    mov eax, 1
    int 0x80

    ; SYS_EXIT()
    mov eax, 4
    int 0x80

.hang:
    jmp .hang
