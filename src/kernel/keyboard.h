#ifndef PROJECT_K_KEYBOARD_H
#define PROJECT_K_KEYBOARD_H

#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 128

void keyboard_initialize(void);
void keyboard_handler(void);

int keyboard_has_input(void);
char keyboard_getchar(void);

#endif
