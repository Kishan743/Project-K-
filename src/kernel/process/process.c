#include "process.h"
#include "../paging.h"

static process_t processes[PROCESS_MAX];
static process_t* current_process;

void process_initialize(void)
{
    for (uint32_t i = 0; i < PROCESS_MAX; i++)
    {
        processes[i].id = i;
        processes[i].state = PROCESS_UNUSED;
        processes[i].address_space = 0;
    }

    processes[0].state = PROCESS_RUNNING;
    processes[0].address_space = paging_get_kernel_address_space();
    current_process = &processes[0];
}

process_t* process_get_current(void)
{
    return current_process;
}

process_t* process_get_table(void)
{
    return processes;
}

process_id_t process_get_current_id(void)
{
    return current_process != 0 ? current_process->id : 0;
}

process_t* process_create_kernel(void)
{
    for (uint32_t i = 1; i < PROCESS_MAX; i++)
    {
        if (processes[i].state == PROCESS_UNUSED)
        {
            processes[i].id = i;
            processes[i].state = PROCESS_READY;
            processes[i].address_space =
                paging_get_kernel_address_space();

            return &processes[i];
        }
    }

    return 0;
}

process_t* process_create_user(address_space_t* address_space)
{
    if (address_space == 0)
        return 0;

    for (uint32_t i = 1; i < PROCESS_MAX; i++)
    {
        if (processes[i].state == PROCESS_UNUSED)
        {
            processes[i].id = i;
            processes[i].state = PROCESS_READY;
            processes[i].address_space = address_space;

            return &processes[i];
        }
    }

    return 0;
}

void process_terminate(process_t* process)
{
    if (process != 0)
        process->state = PROCESS_TERMINATED;
}

void process_destroy(process_t* process)
{
    if (process == 0 || process->id == 0)
        return;

    if (process->address_space != 0 &&
        process->address_space != paging_get_kernel_address_space())
    {
        paging_destroy_address_space(process->address_space);
    }

    process->address_space = 0;
    process->state = PROCESS_UNUSED;
}
