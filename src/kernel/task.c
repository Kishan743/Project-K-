#include "task.h"
#include "heap.h"
#include "gdt.h"
#include "tss.h"
#include "paging.h"
#include "pmm.h"

#define USER_CODE_ADDRESS  0x40000000
#define USER_STACK_ADDRESS 0x40001000
#define USER_STACK_TOP     0x40002000

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

static void user_program_install(void)
{
    /*
     * Ring-3 syscall ABI test:
     *
     * 1. SYS_WRITE_CHAR -> print 'U'
     * 2. SYS_GETPID     -> EAX = current task ID
     * 3. Convert PID to ASCII and print it
     * 4. SYS_YIELD      -> repeatedly yield CPU
     */
    static const uint8_t program[] =
    {
        /* mov eax, SYS_WRITE_CHAR */
        0xB8, 0x01, 0x00, 0x00, 0x00,

        /* mov ebx, 'U' */
        0xBB, 0x55, 0x00, 0x00, 0x00,

        /* int 0x80 */
        0xCD, 0x80,

        /* mov eax, SYS_GETPID */
        0xB8, 0x02, 0x00, 0x00, 0x00,

        /* int 0x80 */
        0xCD, 0x80,

        /* add eax, '0' */
        0x83, 0xC0, 0x30,

        /* mov ebx, eax */
        0x89, 0xC3,

        /* mov eax, SYS_WRITE_CHAR */
        0xB8, 0x01, 0x00, 0x00, 0x00,

        /* int 0x80 */
        0xCD, 0x80,

        /* mov eax, SYS_EXIT */
        0xB8, 0x04, 0x00, 0x00, 0x00,

        /* int 0x80 */
        0xCD, 0x80
    };

    uint8_t* destination = (uint8_t*)USER_CODE_ADDRESS;

    for (uint32_t i = 0;
         i < sizeof(program);
         i++)
    {
        destination[i] = program[i];
    }
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
        tasks[i].type = TASK_KERNEL;
        tasks[i].user_entry = 0;
        tasks[i].user_stack = 0;
        tasks[i].switches = 0;
        tasks[i].work_counter = 0;
    }

    /*
     * Task 0 is the existing kernel execution context.
     */
    tasks[0].state = TASK_RUNNING;
    tasks[0].type = TASK_KERNEL;

    task_count = 1;
    current_task = &tasks[0];
}

int task_create(
    task_entry_t entry,
    void* argument
)
{
    if (entry == 0)
        return -1;

    if (task_count >= TASK_MAX)
        return -1;

    task_t* task = 0;

    for (uint32_t i = 1;
         i < TASK_MAX;
         i++)
    {
        if (tasks[i].state == TASK_UNUSED ||
            tasks[i].state == TASK_TERMINATED)
        {
            task = &tasks[i];
            break;
        }
    }

    if (task == 0)
        return -1;

    void* stack = kmalloc(TASK_STACK_SIZE);

    if (stack == 0)
        return -1;

    task->id = (uint32_t)(task - tasks);
    task->state = TASK_READY;
    task->entry = entry;
    task->argument = argument;
    task->stack = stack;
    task->type = TASK_KERNEL;
    task->user_entry = 0;
    task->user_stack = 0;
    task->switches = 0;
    task->work_counter = 0;

    uint32_t stack_top =
        ((uint32_t)stack + TASK_STACK_SIZE) & ~0x0F;

    uint32_t* sp = (uint32_t*)stack_top;

    /*
     * Synthetic interrupt frame:
     *
     *   EIP
     *   CS
     *   EFLAGS
     *   error code
     *   interrupt number
     *
     * followed by the PUSHA register frame.
     *
     * irq_common will eventually execute:
     *
     *   popa
     *   add esp, 8
     *   iretd
     */

    *(--sp) = 0x202;                  /* EFLAGS */
    *(--sp) = GDT_KERNEL_CODE;        /* CS */
    *(--sp) = (uint32_t)task_bootstrap; /* EIP */
    *(--sp) = 0;                      /* error code */
    *(--sp) = 32;                     /* interrupt number */

    *(--sp) = 0; /* EDI */
    *(--sp) = 0; /* ESI */
    *(--sp) = 0; /* EBP */
    *(--sp) = 0; /* saved ESP */
    *(--sp) = 0; /* EBX */
    *(--sp) = 0; /* EDX */
    *(--sp) = 0; /* ECX */
    *(--sp) = 0; /* EAX */

    task->esp = (uint32_t)sp;

    task_count++;

    return (int)task->id;
}

int task_create_user(
    uint32_t user_entry,
    uint32_t user_stack
)
{
    if (task_count >= TASK_MAX)
        return -1;

    task_t* task = 0;

    for (uint32_t i = 1;
         i < TASK_MAX;
         i++)
    {
        if (tasks[i].state == TASK_UNUSED ||
            tasks[i].state == TASK_TERMINATED)
        {
            task = &tasks[i];
            break;
        }
    }

    if (task == 0)
        return -1;

    uint32_t code_frame = (uint32_t)pmm_alloc_frame();

    if (code_frame == 0)
        return -1;

    uint32_t stack_frame = (uint32_t)pmm_alloc_frame();

    if (stack_frame == 0)
        return -1;

    if (paging_map_page(
            USER_CODE_ADDRESS,
            code_frame,
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER) != 0)
    {
        return -1;
    }

    if (paging_map_page(
            USER_STACK_ADDRESS,
            stack_frame,
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER) != 0)
    {
        return -1;
    }

    user_program_install();

    void* kernel_stack = kmalloc(TASK_STACK_SIZE);

    if (kernel_stack == 0)
        return -1;

    task->id = (uint32_t)(task - tasks);
    task->esp = 0;
    task->state = TASK_READY;
    task->entry = 0;
    task->argument = 0;
    task->stack = kernel_stack;
    task->type = TASK_USER;
    task->user_entry = user_entry;
    task->user_stack = user_stack;
    task->switches = 0;
    task->work_counter = 0;

    uint32_t stack_top =
        ((uint32_t)kernel_stack + TASK_STACK_SIZE) & ~0x0F;

    uint32_t* sp = (uint32_t*)stack_top;

    /*
     * Ring-3 IRET frame:
     *
     *   EIP
     *   CS
     *   EFLAGS
     *   USERESP
     *   USERSS
     *
     * Below that:
     *
     *   error code
     *   interrupt number
     *   PUSHA registers
     */

    *(--sp) = GDT_USER_DATA;       /* USERSS */
    *(--sp) = user_stack;          /* USERESP */
    *(--sp) = 0x202;               /* EFLAGS */
    *(--sp) = GDT_USER_CODE;       /* CS */
    *(--sp) = user_entry;          /* EIP */

    *(--sp) = 0;                   /* error code */
    *(--sp) = 32;                  /* interrupt number */

    *(--sp) = 0; /* EDI */
    *(--sp) = 0; /* ESI */
    *(--sp) = 0; /* EBP */
    *(--sp) = 0; /* saved ESP */
    *(--sp) = 0; /* EBX */
    *(--sp) = 0; /* EDX */
    *(--sp) = 0; /* ECX */
    *(--sp) = 0; /* EAX */

    task->esp = (uint32_t)sp;

    task_count++;

    return (int)task->id;
}

cpu_context_t* task_schedule(
    cpu_context_t* current_context
)
{
    if (current_task != 0)
    {
        current_task->esp = (uint32_t)current_context;

        if (current_task->state == TASK_RUNNING)
            current_task->state = TASK_READY;
    }

    task_t* next = 0;

    uint32_t start = current_task != 0
        ? current_task->id
        : 0;

    for (uint32_t offset = 1;
         offset <= TASK_MAX;
         offset++)
    {
        uint32_t index =
            (start + offset) % TASK_MAX;

        if (tasks[index].state == TASK_READY)
        {
            next = &tasks[index];
            break;
        }
    }

    if (next == 0)
    {
        if (current_task != 0)
            current_task->state = TASK_RUNNING;

        return current_context;
    }

    current_task = next;
    current_task->state = TASK_RUNNING;
    current_task->switches++;

    if (current_task->type == TASK_USER)
    {
        uint32_t kernel_stack_top =
            ((uint32_t)current_task->stack +
             TASK_STACK_SIZE) & ~0x0F;

        tss_set_kernel_stack(kernel_stack_top);
    }

    return (cpu_context_t*)current_task->esp;
}

cpu_context_t* task_yield(
    cpu_context_t* current_context
)
{
    return task_schedule(current_context);
}

cpu_context_t* task_exit_syscall(
    cpu_context_t* current_context
)
{
    if (current_task == 0)
        return current_context;

    current_task->state = TASK_TERMINATED;

    return task_schedule(current_context);
}

void task_exit(void)
{
    if (current_task == 0)
        return;

    current_task->state = TASK_TERMINATED;

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
