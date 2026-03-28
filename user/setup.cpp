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

extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

int main() {
    printf("Carley Setup: Iniciando instalador...\n");

    ipc_msg_t msg;
    msg.sender = 3001;
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 150; msg.data[1] = 100; msg.data[2] = 500; msg.data[3] = 400;
    syscall3(1, 1, (long)&msg, 0);

    msg.type = COMPOSER_DRAW_CHAR;
    msg.data[3] = 0xFFFFFF;

    char *lines[] = {
        "Bienvenido a Carley OS",
        "----------------------",
        "Seleccione su idioma:",
        "[1] Español",
        "[2] English",
        "",
        "Instalando nucleo en /disk...",
        "Configurando SMP y drivers...",
        "",
        "Presione [ENTER] para finalizar."
    };

    for (int i = 0; i < 10; i++) {
        msg.data[1] = 170; msg.data[2] = 150 + (i * 25);
        for(int j=0; lines[i][j]; j++) {
            msg.data[0] = lines[i][j];
            syscall3(1, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }
    }

    while(1) {
        __asm__ volatile("int $0x80" : : "a"(0));
    }
    return 0;
}
