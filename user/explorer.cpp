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

extern "C" long syscall1(int num, long arg1);
extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

typedef struct {
    char name[128];
    uint32_t size;
    uint32_t type;
} vfs_dirent_t;

int main() {
    printf("Carley Explorer: Iniciando...\n");

    // Crear ventana del explorador
    ipc_msg_t msg;
    msg.sender = 2001;
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 50; msg.data[1] = 50; msg.data[2] = 400; msg.data[3] = 300;
    syscall3(1, 1, (long)&msg, 0);

    // Listar archivos en la ventana
    long fd = syscall1(3, (long)"/"); // SYS_OPEN "/"
    if (fd >= 0) {
        vfs_dirent_t dirent;
        int index = 0;
        int y_offset = 80;

        while (syscall3(7, fd, index, (long)&dirent) == 0) { // SYS_READDIR
            msg.type = COMPOSER_DRAW_CHAR;
            msg.data[1] = 70; msg.data[2] = y_offset; msg.data[3] = 0xFFFFFF;

            // Icono simple (cuadradito)
            msg.type = COMPOSER_DRAW_RECT;
            msg.data[0] = 55; msg.data[1] = y_offset - 5; msg.data[2] = 10; msg.data[3] = 10;
            msg.data[4] = (dirent.type == 2) ? 0x3498DB : 0xBDC3C7;
            syscall3(1, 1, (long)&msg, 0);

            // Nombre del archivo
            msg.type = COMPOSER_DRAW_CHAR;
            for(int i=0; dirent.name[i]; i++) {
                msg.data[0] = dirent.name[i];
                syscall3(1, 1, (long)&msg, 0);
                msg.data[1] += 8;
            }

            y_offset += 20;
            index++;
            if (y_offset > 320) break;
        }
    }

    while(1) {
        __asm__ volatile("int $0x80" : : "a"(0));
    }
    return 0;
}
