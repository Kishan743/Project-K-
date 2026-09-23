#include "task.h"
#include "heap.h"
#include "../drivers/terminal.h"

extern void context_switch(
    uint32_t* old_esp,
    uint32_t new_esp
);

static task_t tasks[TASK_MAX];

static uint32_t task_count;
static task_t* current_task;

static void task_bootstrap(void)
{
    task_t* task = current_task;

    if (task != 0 && task->entry != 0)
        task->entry(task->argument);

    task_exit();

    /*
     * task_exit() should never return.
     * Keep the CPU here if something goes wrong.
     */
    while (1)
        __asm__ volatile ("hlt");
}

static int find_next_task(void)
{
    if (current_task == 0)
        return -1;

    uint32_t current_index =
        current_task->id;

    for (uint32_t offset = 1;
         offset < TASK_MAX;
         offset++)
    {
        uint32_t index =
            (current_index + offset) % TASK_MAX;

        if (tasks[index].state == TASK_READY)
            return (int)index;
    }

    return -1;
}

void task_initialize(void)
{
    for (uint32_t i = 0;
         i < TASK_MAX;
         i++)
    {
        tasks[i].id = i;
        tasks[i].esp = 0;
        tasks[i].state = TASK_UNUSED;
        tasks[i].entry = 0;
        tasks[i].argument = 0;
        tasks[i].stack = 0;
    }

    /*
     * Task 0 represents the existing kernel execution
     * context. Its stack pointer is captured the first
     * time task_yield() switches away from it.
     */
    tasks[0].state = TASK_RUNNING;

    current_task = &tasks[0];
    task_count = 1;
}

int task_create(
    task_entry_t entry,
    void* argument
)
{
    if (entry == 0)
        return -1;

    uint32_t slot = TASK_MAX;

    for (uint32_t i = 1;
         i < TASK_MAX;
         i++)
    {
        if (tasks[i].state == TASK_UNUSED ||
            tasks[i].state == TASK_TERMINATED)
        {
            slot = i;
            break;
        }
    }

    if (slot == TASK_MAX)
        return -1;

    void* stack =
        kmalloc(TASK_STACK_SIZE);

    if (stack == 0)
        return -1;

    /*
     * Start at the top of the allocated stack.
     * Keep it 16-byte aligned.
     */
    uint32_t stack_top =
        ((uint32_t)stack +
         TASK_STACK_SIZE) & ~0x0F;

    /*
     * context_switch() expects this layout:
     *
     * ESP -> saved EDI
     *        saved ESI
     *        saved EBX
     *        saved EBP
     *        return address
     */
    uint32_t* initial_stack =
        (uint32_t*)stack_top;

    *(--initial_stack) =
        (uint32_t)task_bootstrap; /* return EIP */

    *(--initial_stack) = 0; /* EBP */
    *(--initial_stack) = 0; /* EBX */
    *(--initial_stack) = 0; /* ESI */
    *(--initial_stack) = 0; /* EDI */

    tasks[slot].id = slot;
    tasks[slot].esp =
        (uint32_t)initial_stack;
    tasks[slot].state = TASK_READY;
    tasks[slot].entry = entry;
    tasks[slot].argument = argument;
    tasks[slot].stack = stack;

    task_count++;

    return (int)slot;
}

void task_yield(void)
{
    int next_index =
        find_next_task();

    if (next_index < 0)
        return;

    task_t* previous =
        current_task;

    task_t* next =
        &tasks[next_index];

    previous->state = TASK_READY;
    next->state = TASK_RUNNING;

    current_task = next;

    context_switch(
        &previous->esp,
        next->esp
    );
}

void task_exit(void)
{
    if (current_task == 0)
        return;

    current_task->state =
        TASK_TERMINATED;

    if (task_count > 0)
        task_count--;

    /*
     * Select another runnable task.
     */
    int next_index = -1;

    for (uint32_t i = 0;
         i < TASK_MAX;
         i++)
    {
        if (&tasks[i] == current_task)
            continue;

        if (tasks[i].state == TASK_READY ||
            tasks[i].state == TASK_RUNNING)
        {
            next_index = (int)i;
            break;
        }
    }

    if (next_index < 0)
    {
        /*
         * No other task exists.
         */
        while (1)
            __asm__ volatile ("hlt");
    }

    task_t* next =
        &tasks[next_index];

    next->state = TASK_RUNNING;
    current_task = next;

    /*
     * There is no useful return address for a
     * terminated task. Switch directly.
     */
    uint32_t ignored_esp = 0;

    context_switch(
        &ignored_esp,
        next->esp
    );

    while (1)
        __asm__ volatile ("hlt");
}

uint32_t task_get_current_id(void)
{
    if (current_task == 0)
        return 0;

    return current_task->id;
}

uint32_t task_get_count(void)
{
    return task_count;
}

const task_t* task_get_current(void)
{
    return current_task;
}
