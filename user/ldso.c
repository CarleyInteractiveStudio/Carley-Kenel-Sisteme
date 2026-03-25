#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ld-carley.so: Enlazador Dinamico del Sistema */

void main(int argc, char **argv) {
    if (argc < 2) {
        printf("LdCarley: No se especifico el ejecutable.\n");
        return;
    }

    printf("LdCarley: Cargando dependencias para %s...\n", argv[1]);

    /* En un sistema real, parseariamos la tabla DYNAMIC y cargariamos libc.so
       usando mmap, luego resolveriamos los simbolos (GOT/PLT). */

    printf("LdCarley: Resolviendo simbolos de libc.so...\n");
    printf("LdCarley: Saltando al punto de entrada del programa...\n");
}
