#include "keyboard_buf.h"
#include "spinlock.h"

static char kbd_buffer[KBD_BUF_SIZE];
static uint32_t head = 0;
static uint32_t tail = 0;
static spinlock_t kbd_lock = 0;

void kbd_buf_init(void) {
    head = tail = 0;
    kbd_lock = 0;
}

void kbd_buf_push(char c) {
    spin_lock(&kbd_lock);
    uint32_t next = (head + 1) % KBD_BUF_SIZE;
    if (next != tail) {
        kbd_buffer[head] = c;
        head = next;
    }
    spin_unlock(&kbd_lock);
}

char kbd_buf_pop(void) {
    spin_lock(&kbd_lock);
    if (head == tail) {
        spin_unlock(&kbd_lock);
        return 0;
    }
    char c = kbd_buffer[tail];
    tail = (tail + 1) % KBD_BUF_SIZE;
    spin_unlock(&kbd_lock);
    return c;
}

size_t kbd_buf_read(char *buffer, size_t size) {
    size_t count = 0;
    while (count < size) {
        char c = kbd_buf_pop();
        if (c == 0) break;
        buffer[count++] = c;
    }
    return count;
}
