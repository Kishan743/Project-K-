#ifndef PROJECT_K_KEYBOARD_H
#define PROJECT_K_KEYBOARD_H

#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 128

#define KEYBOARD_OWNER_KERNEL 0
#define KEYBOARD_OWNER_USER   1

void keyboard_initialize(void);
void keyboard_handler(void);

int keyboard_has_input(void);
char keyboard_getchar(void);

void keyboard_set_owner(uint32_t owner);
uint32_t keyboard_get_owner(void);

#endif
