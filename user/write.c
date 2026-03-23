#include "include/stdio.h"
#include "include/string.h"
#include <stdint.h>

extern long syscall1(int num, long arg1);
extern long syscall3(int num, long arg1, long arg2, long arg3);

#define SYS_OPEN  3
#define SYS_WRITE 6

void main(void) {
    printf("Carley Writer v0.1\n");

    /* Intentar abrir el archivo de prueba en RamFS */
    void *file = (void *)syscall1(SYS_OPEN, (long)"ram/test.txt");

    if (file) {
        const char *data = "Hola Carley Kernel! Este texto se guardo en la RAM.";
        syscall3(SYS_WRITE, (long)file, (long)data, strlen(data));
        printf("Escritura exitosa en ram/test.txt\n");
    } else {
        printf("Error: No se pudo abrir ram/test.txt\n");
    }
}
