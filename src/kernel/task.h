#ifndef PROJECT_K_TASK_H
#define PROJECT_K_TASK_H

#include <stdint.h>

#define TASK_MAX        8
#define TASK_STACK_SIZE (16 * 1024)

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

} task_t;

void task_initialize(void);

int task_create(
    task_entry_t entry,
    void* argument
);

void task_yield(void);

void task_exit(void);

uint32_t task_get_current_id(void);

uint32_t task_get_count(void);

const task_t* task_get_current(void);

#endif
