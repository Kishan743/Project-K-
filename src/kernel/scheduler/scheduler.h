#ifndef PROJECT_K_SCHEDULER_H
#define PROJECT_K_SCHEDULER_H

#include <stdint.h>

typedef struct cpu_context cpu_context_t;
typedef struct thread thread_t;

void scheduler_initialize(void);

void scheduler_add_thread(thread_t* thread);
void scheduler_remove_thread(thread_t* thread);

cpu_context_t* scheduler_schedule(
    cpu_context_t* current_context
);

cpu_context_t* scheduler_yield(
    cpu_context_t* current_context
);

void scheduler_block(thread_t* thread);
void scheduler_unblock(thread_t* thread);

#endif
