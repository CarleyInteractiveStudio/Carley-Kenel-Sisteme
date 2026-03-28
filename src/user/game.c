#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ipc.h>

extern long syscall3(int num, long arg1, long arg2, long arg3);

#define SYS_IPC_SEND 1
#define COMPOSER_DRAW_RECT 2
#define COMPOSER_CLEAR 4

void main(void) {
    int x = 50, y = 50;
    int dx = 5, dy = 5;

    printf("Iniciando Carley Game (Demo)...\n");

    for (int frame = 0; frame < 500; frame++) {
        ipc_msg_t msg;

        /* Limpiar cuadro anterior (borrado rapido) */
        msg.type = COMPOSER_DRAW_RECT;
        msg.data[0] = x; msg.data[1] = y; msg.data[2] = 20; msg.data[3] = 20; msg.data[4] = 0x1E1E1E;
        syscall3(SYS_IPC_SEND, 0, (long)&msg, 0);

        x += dx; y += dy;
        if (x < 10 || x > 300) dx = -dx;
        if (y < 10 || y > 300) dy = -dy;

        /* Dibujar bola */
        msg.data[0] = x; msg.data[1] = y; msg.data[4] = 0x3498DB;
        syscall3(SYS_IPC_SEND, 0, (long)&msg, 0);

        /* Delay frame */
        for(int j=0; j<500000; j++) __asm__("pause");

        __asm__ volatile("int $0x80" : : "a"(0)); // Yield
    }

    printf("Demo terminada.\n");
}
