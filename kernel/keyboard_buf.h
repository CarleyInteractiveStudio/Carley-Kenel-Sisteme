#ifndef KEYBOARD_BUF_H
#define KEYBOARD_BUF_H

#include <stdint.h>
#include <stddef.h>

#define KBD_BUF_SIZE 1024

void kbd_buf_init(void);
void kbd_buf_push(char c);
char kbd_buf_pop(void);
size_t kbd_buf_read(char *buffer, size_t size);

#endif
