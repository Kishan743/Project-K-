#include "thread.h"
#include "../heap.h"

static thread_t threads[THREAD_MAX];
static thread_t* current_thread;

void thread_initialize(void)
{
    for (uint32_t i = 0; i < THREAD_MAX; i++)
    {
        threads[i].id = i;
        threads[i].state = THREAD_UNUSED;
        threads[i].process = 0;
        threads[i].esp = 0;
        threads[i].kernel_stack = 0;
        threads[i].entry = 0;
        threads[i].argument = 0;
        threads[i].switches = 0;
        threads[i].work_counter = 0;
    }

    threads[0].state = THREAD_RUNNING;
    current_thread = &threads[0];
}

thread_t* thread_get_current(void)
{
    return current_thread;
}

thread_t* thread_get_table(void)
{
    return threads;
}

thread_t* thread_create(
    process_t* process,
    thread_entry_t entry,
    void* argument
)
{
    if (process == 0)
        return 0;

    for (uint32_t i = 1; i < THREAD_MAX; i++)
    {
        if (threads[i].state == THREAD_UNUSED)
        {
            void* stack = kmalloc(THREAD_STACK_SIZE);

            if (stack == 0)
                return 0;

            threads[i].id = i;
            threads[i].state = THREAD_READY;
            threads[i].process = process;
            threads[i].esp = 0;
            threads[i].kernel_stack = stack;
            threads[i].entry = entry;
            threads[i].argument = argument;
            threads[i].switches = 0;
            threads[i].work_counter = 0;

            return &threads[i];
        }
    }

    return 0;
}

void thread_terminate(thread_t* thread)
{
    if (thread != 0)
        thread->state = THREAD_TERMINATED;
}
