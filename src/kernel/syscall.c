#include "syscall.h"
#include "task.h"
#include "keyboard.h"
#include "../drivers/terminal.h"

static volatile uint32_t syscall_count;

static uint32_t syscall_write_char(uint32_t character)
{
    terminal_putchar((char)character);
    return 0;
}

static uint32_t syscall_getpid(void)
{
    return task_get_current_id();
}

static uint32_t syscall_read_char(void)
{
    if (!keyboard_has_input())
        return 0;

    return (uint32_t)(uint8_t)keyboard_getchar();
}

cpu_context_t* syscall_entry(cpu_context_t* frame)
{
    if (frame == 0)
        return frame;

    syscall_count++;

    switch (frame->eax)
    {
        case SYS_WRITE_CHAR:
            frame->eax = syscall_write_char(frame->ebx);
            return frame;

        case SYS_GETPID:
            frame->eax = syscall_getpid();
            return frame;

        case SYS_READ_CHAR:
            frame->eax = syscall_read_char();
            return frame;

        case SYS_YIELD:
            return task_yield(frame);

        case SYS_EXIT:
            return task_exit_syscall(frame);

        default:
            frame->eax = (uint32_t)-1;
            return frame;
    }
}

uint32_t syscall_get_count(void)
{
    return syscall_count;
}
