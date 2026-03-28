#include <stdint.h>
#include <ipc.h>

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

#define COMPOSER_CREATE_WINDOW 10
#define COMPOSER_DRAW_RECT     2
#define COMPOSER_DRAW_CHAR     3

#define SYS_GET_INFO 11
#define SYS_IPC_SEND 1

extern "C" long syscall1(int num, long arg1);
extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

int main() {
    ipc_msg_t msg;
    msg.sender = 7001;
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 100; msg.data[1] = 100; msg.data[2] = 500; msg.data[3] = 400;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    // Fondo gris estilo ajustes
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = 100; msg.data[1] = 120; msg.data[2] = 500; msg.data[3] = 380;
    msg.data[4] = 0x2C3E50;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    msg.type = COMPOSER_DRAW_CHAR;
    msg.data[3] = 0xFFFFFF;

    uint64_t total_ram = (uint64_t)syscall1(SYS_GET_INFO, 0);

    char *lines[12];
    char l1[] = "  SYSTEM SETTINGS";
    char l2[] = "=================";
    char l3[] = " [STORAGE]";
    char l4[] = "  /disk: CarleyFS - 10MB Total (IDE PIO)";
    char l5[] = "  /ram:  RamFS    - 2MB Total (RW)";
    char l6[] = "";
    char l7[] = " [HARDWARE]";
    char l8[64]; sprintf(l8, "  RAM: %d MB", (int)(total_ram / 1024 / 1024));
    char l9[] = "  CPU: x86_64 SMP (Multi-core)";
    char l10[] = "";
    char l11[] = " [APPEARANCE]";
    char l12[] = "  Theme: Dark Glass (Default)";

    lines[0] = l1; lines[1] = l2; lines[2] = l3; lines[3] = l4;
    lines[4] = l5; lines[5] = l6; lines[6] = l7; lines[7] = l8;
    lines[8] = l9; lines[9] = l10; lines[10] = l11; lines[11] = l12;

    for (int i = 0; i < 12; i++) {
        msg.data[1] = 120; msg.data[2] = 150 + (i * 25);
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
