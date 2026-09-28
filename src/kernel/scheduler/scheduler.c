#include "scheduler.h"
#include "../process/thread.h"
#include "../process/process.h"
#include "../heap.h"

static thread_t* scheduler_current;

void scheduler_initialize(void)
{
    thread_initialize();
    scheduler_current = thread_get_current();
}

void scheduler_add_thread(thread_t* thread)
{
    if (thread != 0 &&
        thread->state == THREAD_UNUSED)
    {
        thread->state = THREAD_READY;
    }
}

void scheduler_remove_thread(thread_t* thread)
{
    if (thread != 0)
        thread->state = THREAD_UNUSED;
}

cpu_context_t* scheduler_schedule(
    cpu_context_t* current_context
)
{
    if (current_context == 0 ||
        scheduler_current == 0)
    {
        return current_context;
    }

    scheduler_current->esp =
        (uint32_t)current_context;

    if (scheduler_current->state == THREAD_RUNNING)
        scheduler_current->state = THREAD_READY;

    thread_t* next = 0;
    uint32_t start = scheduler_current->id;

    for (uint32_t offset = 1;
         offset <= THREAD_MAX;
         offset++)
    {
        uint32_t index =
            (start + offset) % THREAD_MAX;

        if (thread_get_table()[index].state == THREAD_READY)
        {
            next = &thread_get_table()[index];
            break;
        }
    }

    if (next == 0)
    {
        scheduler_current->state = THREAD_RUNNING;
        return current_context;
    }

    thread_t* previous = scheduler_current;

    scheduler_current = next;
    scheduler_current->state = THREAD_RUNNING;
    scheduler_current->switches++;

    if (scheduler_current->process != 0)
    {
        address_space_t* address_space =
            scheduler_current->process->address_space;

        if (address_space != 0)
            paging_switch_address_space(address_space);
    }

    if (previous != 0 &&
        previous != scheduler_current &&
        previous->state == THREAD_TERMINATED)
    {
        if (previous->kernel_stack != 0)
        {
            kfree(previous->kernel_stack);
            previous->kernel_stack = 0;
        }

        previous->state = THREAD_UNUSED;
    }

    return (cpu_context_t*)scheduler_current->esp;
}

cpu_context_t* scheduler_yield(
    cpu_context_t* current_context
)
{
    return scheduler_schedule(current_context);
}

void scheduler_block(thread_t* thread)
{
    if (thread != 0)
        thread->state = THREAD_BLOCKED;
}

void scheduler_unblock(thread_t* thread)
{
    if (thread != 0 &&
        thread->state == THREAD_BLOCKED)
    {
        thread->state = THREAD_READY;
    }
}
