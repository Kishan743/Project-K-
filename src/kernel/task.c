#include "task.h"
#include "heap.h"

static task_t tasks[TASK_MAX];

static uint32_t task_count;
static task_t* current_task;

static void task_bootstrap(void)
{
    task_t* task = current_task;

    if (task != 0 && task->entry != 0)
        task->entry(task->argument);

    task_exit();

    while (1)
        __asm__ volatile ("hlt");
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
        tasks[i].switches = 0;
        tasks[i].work_counter = 0;
    }

    /*
     * Task 0 is the existing kernel execution context.
     *
     * Its real CPU context will be captured automatically
     * the first time IRQ0 fires.
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
     * Stack grows downward.
     *
     * We construct exactly the context that irq_common
     * expects after PUSHA.
     */
    uint32_t stack_top =
        ((uint32_t)stack + TASK_STACK_SIZE) & ~0x0F;

    uint32_t* sp =
        (uint32_t*)stack_top;

    /*
     * iret frame.
     */
    *(--sp) = 0x00000202;       /* EFLAGS: IF enabled */
    *(--sp) = 0x00000008;       /* CS: kernel code */
    *(--sp) = (uint32_t)task_bootstrap;

    /*
     * Software-created interrupt metadata.
     */
    *(--sp) = 0;                /* error code */
    *(--sp) = 32;               /* IRQ0 vector */

    /*
     * PUSHA frame.
     *
     * popa restores:
     * EDI, ESI, EBP, skips ESP,
     * EBX, EDX, ECX, EAX
     */
    *(--sp) = 0;                /* EAX */
    *(--sp) = 0;                /* ECX */
    *(--sp) = 0;                /* EDX */
    *(--sp) = 0;                /* EBX */
    *(--sp) = 0;                /* saved ESP */
    *(--sp) = 0;                /* EBP */
    *(--sp) = 0;                /* ESI */
    *(--sp) = 0;                /* EDI */

    tasks[slot].id = slot;
    tasks[slot].esp = (uint32_t)sp;
    tasks[slot].state = TASK_READY;
    tasks[slot].entry = entry;
    tasks[slot].argument = argument;
    tasks[slot].stack = stack;
    tasks[slot].switches = 0;
    tasks[slot].work_counter = 0;

    task_count++;

    return (int)slot;
}

static int find_next_ready_task(void)
{
    if (current_task == 0)
        return -1;

    uint32_t current_id =
        current_task->id;

    for (uint32_t offset = 1;
         offset < TASK_MAX;
         offset++)
    {
        uint32_t index =
            (current_id + offset) % TASK_MAX;

        if (tasks[index].state == TASK_READY)
            return (int)index;
    }

    return -1;
}

cpu_context_t* task_schedule(
    cpu_context_t* current_context
)
{
    if (current_task == 0)
        return current_context;

    /*
     * Save the interrupted task's current CPU context.
     */
    current_task->esp =
        (uint32_t)current_context;

    int next_index =
        find_next_ready_task();

    if (next_index < 0)
        return current_context;

    task_t* previous =
        current_task;

    task_t* next =
        &tasks[next_index];

    previous->state = TASK_READY;

    next->state = TASK_RUNNING;
    next->switches++;

    current_task = next;

    return (cpu_context_t*)next->esp;
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
     * The timer interrupt will eventually notice that this
     * task is no longer READY and switch away from it.
     *
     * Do not free this task's stack while running on it.
     */
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

task_t* task_get_current(void)
{
    return current_task;
}

const task_t* task_get_table(void)
{
    return tasks;
}
