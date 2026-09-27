BITS 32
%include "src/user/syscall.inc"

GLOBAL _start

SECTION .text

_start:
    ; SYS_WRITE_CHAR('U')
    mov eax, SYS_WRITE_CHAR
    mov ebx, 'U'
    int 0x80

    ; SYS_GETPID()
    mov eax, SYS_GETPID
    int 0x80

    ; Convert PID to one ASCII digit.
    add eax, '0'
    mov ebx, eax

    ; SYS_WRITE_CHAR(pid)
    mov eax, SYS_WRITE_CHAR
    int 0x80

.wait:
    ; SYS_READ_CHAR()
    mov eax, SYS_READ_CHAR
    int 0x80

    ; EAX == 0 means no input.
    test eax, eax
    jz .yield

    ; Echo received character.
    mov ebx, eax
    mov eax, SYS_WRITE_CHAR
    int 0x80

    ; SYS_EXIT()
    mov eax, SYS_EXIT
    int 0x80

.yield:
    mov eax, SYS_YIELD
    int 0x80
    jmp .wait

.hang:
    jmp .hang
