#include "task.h"
#include "heap.h"
#include "gdt.h"
#include "tss.h"
#include "paging.h"
#include "pmm.h"
#include "elf.h"
#include "keyboard.h"
#include "process/process.h"
#include "process/thread.h"
#include "scheduler/scheduler.h"

static uint32_t task_count;

static void task_bootstrap(void)
{
    task_t* task = task_get_current();

    if (task != 0 && task->entry != 0)
        task->entry(task->argument);

    task_exit();

    while (1)
        __asm__ volatile ("hlt");
}

void task_initialize(void)
{
    process_initialize();
    scheduler_initialize();

    task_count = 1;

    task_t* current = task_get_current();

    if (current != 0)
    {
        current->process =
            process_get_current();

        current->type =
            TASK_KERNEL;
    }
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

    process_t* process =
        process_create_kernel();

    if (process == 0)
        return -1;

    task_t* task =
        thread_create(
            process,
            entry,
            argument
        );

    if (task == 0)
    {
        process_destroy(process);
        return -1;
    }

    task->type = TASK_KERNEL;

    uint32_t stack_top =
        ((uint32_t)task->kernel_stack +
         TASK_STACK_SIZE) & ~0x0F;

    uint32_t* sp =
        (uint32_t*)stack_top;

    /*
     * Synthetic interrupt frame:
     *
     * PUSHA registers
     * interrupt number
     * error code
     * EIP
     * CS
     * EFLAGS
     */

    *(--sp) = 0x202;
    *(--sp) = GDT_KERNEL_CODE;
    *(--sp) = (uint32_t)task_bootstrap;
    *(--sp) = 0;
    *(--sp) = 32;

    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    task->esp =
        (uint32_t)sp;

    task_count++;

    return (int)task->id;
}

int task_create_user_elf(
    const void* image,
    uint32_t image_size
)
{
    if (image == 0 || image_size == 0)
        return -1;

    if (task_count >= TASK_MAX)
        return -1;

    address_space_t* address_space =
        paging_create_address_space();

    if (address_space == 0)
        return -1;

    elf_load_result_t load_result;

    if (elf_load(
            address_space,
            image,
            image_size,
            &load_result
        ) != 0)
    {
        paging_destroy_address_space(
            address_space
        );

        return -1;
    }

    const uint32_t user_stack_address =
        0x7FFFF000;

    const uint32_t user_stack_top =
        0x80000000;

    uint32_t stack_frame =
        (uint32_t)pmm_alloc_frame();

    if (stack_frame == 0)
    {
        paging_destroy_address_space(
            address_space
        );

        return -1;
    }

    uint8_t zero_page[PAGE_SIZE];

    for (uint32_t i = 0;
         i < PAGE_SIZE;
         i++)
    {
        zero_page[i] = 0;
    }

    if (paging_copy_to_physical(
            stack_frame,
            zero_page,
            PAGE_SIZE
        ) != 0)
    {
        pmm_free_frame(
            (void*)stack_frame
        );

        paging_destroy_address_space(
            address_space
        );

        return -1;
    }

    if (paging_map_user_page(
            address_space,
            user_stack_address,
            stack_frame,
            PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER
        ) != 0)
    {
        pmm_free_frame(
            (void*)stack_frame
        );

        paging_destroy_address_space(
            address_space
        );

        return -1;
    }

    process_t* process =
        process_create_user(
            address_space
        );

    if (process == 0)
    {
        paging_destroy_address_space(
            address_space
        );

        return -1;
    }

    task_t* task =
        thread_create(
            process,
            0,
            0
        );

    if (task == 0)
    {
        process_destroy(process);
        return -1;
    }

    task->type = TASK_USER;
    task->user_entry = load_result.entry;
    task->user_stack = user_stack_top;

    uint32_t stack_top =
        ((uint32_t)task->kernel_stack +
         TASK_STACK_SIZE) & ~0x0F;

    uint32_t* sp =
        (uint32_t*)stack_top;

    /*
     * Ring-3 IRET frame.
     */

    *(--sp) = GDT_USER_DATA;
    *(--sp) = user_stack_top;
    *(--sp) = 0x202;
    *(--sp) = GDT_USER_CODE;
    *(--sp) = load_result.entry;

    *(--sp) = 0;
    *(--sp) = 32;

    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    task->esp =
        (uint32_t)sp;

    task_count++;

    keyboard_set_owner(
        KEYBOARD_OWNER_USER
    );

    return (int)task->id;
}

cpu_context_t* task_schedule(
    cpu_context_t* current_context
)
{
    return scheduler_schedule(
        current_context
    );
}

cpu_context_t* task_yield(
    cpu_context_t* current_context
)
{
    return scheduler_yield(
        current_context
    );
}

cpu_context_t* task_exit_syscall(
    cpu_context_t* current_context
)
{
    task_t* current =
        task_get_current();

    if (current == 0)
        return current_context;

    current->state =
        TASK_TERMINATED;

    if (current->process != 0)
        process_terminate(
            current->process
        );

    return scheduler_schedule(
        current_context
    );
}

void task_exit(void)
{
    task_t* current =
        task_get_current();

    if (current == 0)
        return;

    current->state =
        TASK_TERMINATED;

    if (current->process != 0)
        process_terminate(
            current->process
        );

    while (1)
        __asm__ volatile ("hlt");
}

uint32_t task_get_current_id(void)
{
    task_t* current =
        task_get_current();

    return current != 0
        ? current->id
        : 0;
}

uint32_t task_get_count(void)
{
    return task_count;
}

task_t* task_get_current(void)
{
    return thread_get_current();
}

const task_t* task_get_table(void)
{
    return thread_get_table();
}
