#include <stdint.h>
#include <ipc.h>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#define COMPOSER_CREATE_WINDOW 10
#define COMPOSER_ATTACH_SHM    12

extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

int main() {
    printf("SHM Demo: Iniciando modo de alto rendimiento...\n");

    uint32_t w = 200, h = 150;

    // 1. Crear ventana
    ipc_msg_t msg;
    msg.sender = 5001;
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 400; msg.data[1] = 300; msg.data[2] = w; msg.data[3] = h + 20;
    syscall3(1, 1, (long)&msg, 0);

    // 2. Pedir Memoria Compartida (ID 123, tamaño para el buffer)
    long shm_id = shm_get(123, w * h * 4);
    uint32_t *buffer = (uint32_t *)shm_at(shm_id, NULL);

    // 3. Vincular buffer al Composer
    msg.type = COMPOSER_ATTACH_SHM;
    msg.data[0] = shm_id;
    syscall3(1, 1, (long)&msg, 0);

    uint32_t frame = 0;
    while (1) {
        // Manipulación directa de píxeles sin IPC
        for (uint32_t i = 0; i < h; i++) {
            for (uint32_t j = 0; j < w; j++) {
                buffer[i * w + j] = (i + frame) ^ (j + frame);
            }
        }
        frame++;

        // Ceder CPU
        __asm__ volatile("int $0x80" : : "a"(0));
    }

    return 0;
}
