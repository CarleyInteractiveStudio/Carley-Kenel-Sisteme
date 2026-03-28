#include <stdint.h>
#include <ipc.h>

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
}

#define COMPOSER_CREATE_WINDOW 10
#define COMPOSER_DRAW_RECT     2
#define COMPOSER_DRAW_CHAR     3

#define SYS_GET_INFO 11
#define SYS_TIME     12
#define SYS_IPC_SEND 1

extern "C" long syscall1(int num, long arg1);
extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

int main() {
    ipc_msg_t msg;
    msg.sender = 5001; // ID para About
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 200; msg.data[1] = 150; msg.data[2] = 400; msg.data[3] = 300;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    // Fondo oscuro estilo Mac
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = 200; msg.data[1] = 170; msg.data[2] = 400; msg.data[3] = 280;
    msg.data[4] = 0x1E1E1E;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    msg.type = COMPOSER_DRAW_CHAR;
    msg.data[3] = 0xFFFFFF;

    uint64_t total_ram = (uint64_t)syscall1(SYS_GET_INFO, 0);
    struct tm t;
    time((time_t *)&t);

    char *lines[10];
    char l1[] = "    CARLEY OS";
    char l2[] = "-------------------";
    char l3[64]; sprintf(l3, "Kernel: v0.5.2-alpha");
    char l4[64]; sprintf(l4, "Memory: %d MB", (int)(total_ram / 1024 / 1024));
    char l5[64]; sprintf(l5, "CPU: x86_64 Multi-Core");
    char l6[64]; sprintf(l6, "Time: %02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    char l7[] = "-------------------";
    char l8[] = "   Made with love";
    char l9[] = "   for gaming & dev";

    lines[0] = l1; lines[1] = l2; lines[2] = l3; lines[3] = l4;
    lines[4] = l5; lines[5] = l6; lines[6] = l7; lines[7] = l8; lines[8] = l9;

    for (int i = 0; i < 9; i++) {
        msg.data[1] = 240; msg.data[2] = 200 + (i * 25);
        for(int j=0; lines[i][j]; j++) {
            msg.data[0] = lines[i][j];
            syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }
    }

    while(1) {
        __asm__ volatile("int $0x80" : : "a"(0));
    }
    return 0;
}
