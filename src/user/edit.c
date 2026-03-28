#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

extern long syscall3(int num, long arg1, long arg2, long arg3);
#define SYS_READ 4

void main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    char *filename = "ram/edit.txt";
    printf("Carley Edit v0.1 - Editando: %s\n", filename);
    printf("Escribe tu texto (Pulsa ESC para guardar y salir):\n");

    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Error: No se pudo crear el archivo.\n");
        return;
    }

    char c;
    while (1) {
        /* Leer de stdin */
        if (syscall3(SYS_READ, 0, (long)&c, 1) > 0) {
            if (c == 27) break; // ESC
            if (c == '\n') putchar('\n');
            else putchar(c);

            fwrite(&c, 1, 1, f);
        }
        /* Yield manual */
        __asm__ volatile("int $0x80" : : "a"(0));
    }

    fclose(f);
    printf("\nArchivo guardado exitosamente.\n");
}
