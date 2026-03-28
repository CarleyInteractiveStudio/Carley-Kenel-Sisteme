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
    printf("Carley Code: Iniciando editor...\n");

    ipc_msg_t msg;
    msg.sender = 4001;
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 100; msg.data[1] = 80; msg.data[2] = 600; msg.data[3] = 450;
    syscall3(1, 1, (long)&msg, 0);

    // Dibujar fondo oscuro (estilo VS Code)
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = 100; msg.data[1] = 100; msg.data[2] = 600; msg.data[3] = 430;
    msg.data[4] = 0x1E1E1E;
    syscall3(1, 1, (long)&msg, 0);

    msg.type = COMPOSER_DRAW_CHAR;
    msg.data[3] = 0x569CD6; // Azul palabras clave

    char *code[] = {
        "#include <stdio.h>",
        "",
        "int main() {",
        "    printf(\"Hola Carley OS!\\n\");",
        "    return 0;",
        "}"
    };

    for (int i = 0; i < 6; i++) {
        msg.data[1] = 120; msg.data[2] = 120 + (i * 20);
        for(int j=0; code[i][j]; j++) {
            msg.data[0] = code[i][j];
            syscall3(1, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }
    }

    while(1) {
        __asm__ volatile("int $0x80" : : "a"(0));
    }
    return 0;
}
