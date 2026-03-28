#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* C-Run: Ejecuta un binario crudo cargado en memoria */

void main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: crun [archivo.bin]\n");
        return;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) { printf("Error: No se pudo abrir %s\n", argv[1]); return; }

    /* Pedir memoria para el binario */
    void *mem = malloc(4096);
    if (!mem) { printf("Error de memoria.\n"); fclose(f); return; }

    size_t n = fread(mem, 1, 4096, f);
    printf("Cargados %d bytes. Ejecutando...\n", n);

    /* Castear a puntero de función y ejecutar */
    void (*code)(void) = (void (*)(void))mem;
    code();

    printf("\nEjecucion terminada.\n");
    fclose(f);
}
