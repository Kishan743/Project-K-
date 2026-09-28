#ifndef PROJECT_K_PROCESS_H
#define PROJECT_K_PROCESS_H

#include <stdint.h>
#include "../paging.h"

#define PROCESS_MAX 8

typedef uint32_t process_id_t;

typedef enum
{
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process
{
    process_id_t id;
    process_state_t state;
    address_space_t* address_space;
} process_t;

void process_initialize(void);
process_t* process_get_current(void);
process_t* process_get_table(void);
process_id_t process_get_current_id(void);
process_t* process_create_kernel(void);
process_t* process_create_user(address_space_t* address_space);
void process_terminate(process_t* process);
void process_destroy(process_t* process);

#endif
