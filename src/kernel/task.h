#ifndef PROJECT_K_TASK_H
#define PROJECT_K_TASK_H

#include "arch/cpu.h"
#include "process/process.h"
#include "process/thread.h"

#define TASK_MAX        THREAD_MAX
#define TASK_STACK_SIZE THREAD_STACK_SIZE

#define TASK_KERNEL THREAD_KERNEL
#define TASK_USER   THREAD_USER

typedef thread_t task_t;
typedef thread_state_t task_state_t;
typedef thread_entry_t task_entry_t;

#define TASK_UNUSED     THREAD_UNUSED
#define TASK_READY      THREAD_READY
#define TASK_RUNNING    THREAD_RUNNING
#define TASK_BLOCKED    THREAD_BLOCKED
#define TASK_TERMINATED THREAD_TERMINATED

void task_initialize(void);

int task_create(
    task_entry_t entry,
    void* argument
);

int task_create_user_elf(
    const void* image,
    uint32_t image_size
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
