#ifndef PROJECT_K_TASK_H
#define PROJECT_K_TASK_H

#include <stdint.h>

#define TASK_MAX        8
#define TASK_STACK_SIZE (16 * 1024)

#define TASK_KERNEL 0
#define TASK_USER   1

/*
 * This structure matches the stack produced by
 * PUSHA in irq_common, followed by the normalized
 * interrupt frame.
 *
 * useresp/userss are present when the interrupted
 * task originated from Ring 3.
 */
typedef struct cpu_context
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t saved_esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t interrupt_number;
    uint32_t error_code;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

    uint32_t useresp;
    uint32_t userss;

} cpu_context_t;

typedef enum
{
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_TERMINATED

} task_state_t;

typedef void (*task_entry_t)(void* argument);

typedef struct task
{
    uint32_t id;
    uint32_t esp;

    task_state_t state;

    task_entry_t entry;
    void* argument;
    void* stack;

    uint32_t type;

    uint32_t user_entry;
    uint32_t user_stack;

    volatile uint32_t switches;
    volatile uint32_t work_counter;

} task_t;

void task_initialize(void);

int task_create(
    task_entry_t entry,
    void* argument
);

int task_create_user(
    uint32_t user_entry,
    uint32_t user_stack
);

cpu_context_t* task_schedule(
    cpu_context_t* current_context
);

cpu_context_t* task_yield(
    cpu_context_t* current_context
);

cpu_context_t* task_exit_syscall(
    cpu_context_t* current_context
);

void task_exit(void);

uint32_t task_get_current_id(void);
uint32_t task_get_count(void);

task_t* task_get_current(void);
const task_t* task_get_table(void);

#endif
