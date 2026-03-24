#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* CarleyAsm: Convierte texto hexadecimal a binario ejecutable */

uint8_t hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

void main(int argc, char **argv) {
    if (argc < 3) {
        printf("Uso: casm [origen.txt] [destino.bin]\n");
        return;
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) { printf("Error: No se pudo abrir el origen.\n"); return; }

    FILE *out = fopen(argv[2], "w");
    if (!out) { printf("Error: No se pudo abrir el destino.\n"); fclose(in); return; }

    printf("Ensamblando %s -> %s...\n", argv[1], argv[2]);

    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, 511, in)) > 0) {
        buf[n] = 0;
        for (size_t i = 0; i < n; i++) {
            if (isspace(buf[i])) continue;

            /* Leer dos caracteres hex */
            uint8_t val = (hex_to_int(buf[i]) << 4) | hex_to_int(buf[i+1]);
            fwrite(&val, 1, 1, out);
            i++;
        }
    }

    fclose(in);
    fclose(out);
    printf("Ensamblado completado.\n");
}
