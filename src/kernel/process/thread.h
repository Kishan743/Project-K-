#ifndef PROJECT_K_THREAD_H
#define PROJECT_K_THREAD_H

#include <stdint.h>
#include "process.h"

#define THREAD_MAX 16
#define THREAD_STACK_SIZE (16 * 1024)

#define THREAD_KERNEL 0
#define THREAD_USER   1

typedef enum
{
    THREAD_UNUSED = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

typedef void (*thread_entry_t)(void* argument);

typedef struct cpu_context cpu_context_t;

typedef struct thread
{
    uint32_t id;
    thread_state_t state;

    process_t* process;

    uint32_t esp;
    void* kernel_stack;

    thread_entry_t entry;
    void* argument;

    uint32_t type;
    uint32_t user_entry;
    uint32_t user_stack;

    volatile uint32_t switches;
    volatile uint32_t work_counter;
} thread_t;

void thread_initialize(void);

thread_t* thread_get_current(void);
thread_t* thread_get_table(void);

thread_t* thread_create(
    process_t* process,
    thread_entry_t entry,
    void* argument
);

void thread_terminate(thread_t* thread);

#endif
