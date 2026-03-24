#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ipc.h>

extern long syscall3(int num, long arg1, long arg2, long arg3);

#define SYS_IPC_SEND 1
#define COMPOSER_DRAW_RECT 2
#define COMPOSER_DRAW_CHAR 3

void gui_draw_button(int x, int y, const char *label, uint32_t color) {
    ipc_msg_t msg;
    msg.sender = 200;
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = x; msg.data[1] = y; msg.data[2] = 80; msg.data[3] = 30; msg.data[4] = color;
    syscall3(SYS_IPC_SEND, 0, (long)&msg, 0);

    msg.type = COMPOSER_DRAW_CHAR;
    msg.data[1] = x + 10; msg.data[2] = y + 10; msg.data[3] = 0xFFFFFF;
    for (int i=0; label[i]; i++) {
        msg.data[0] = label[i];
        syscall3(SYS_IPC_SEND, 0, (long)&msg, 0);
        msg.data[1] += 8;
    }
}

void main(void) {
    printf("Carley UI v0.1 cargando...\n");

    gui_draw_button(50, 50, "HELLO", 0x27AE60);
    gui_draw_button(150, 50, "EDIT", 0x2980B9);
    gui_draw_button(250, 50, "EXIT", 0xC0392B);

    printf("Interfaz grafica iniciada.\n");
    for (;;) {
        __asm__ volatile("int $0x80" : : "a"(0));
    }
}
