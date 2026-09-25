#ifndef PROJECT_K_SYSCALL_H
#define PROJECT_K_SYSCALL_H

#include <stdint.h>

typedef struct cpu_context cpu_context_t;

#define SYS_WRITE_CHAR 1
#define SYS_GETPID     2
#define SYS_YIELD      3
#define SYS_EXIT       4

cpu_context_t* syscall_entry(cpu_context_t* frame);

uint32_t syscall_get_count(void);

#endif
