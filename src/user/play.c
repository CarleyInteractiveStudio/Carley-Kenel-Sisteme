#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

extern long syscall2(int num, long arg1, long arg2);
#define SYS_AUDIO_PLAY 13

void main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: play [archivo.raw]\n");
        return;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) { printf("Error: No se pudo abrir %s\n", argv[1]); return; }
    uint8_t *buffer = malloc(4096);
    if (!buffer) { fclose(f); return; }
    printf("Reproduciendo %s...\n", argv[1]);
    size_t n;
    while ((n = fread(buffer, 1, 4096, f)) > 0) {
        syscall2(SYS_AUDIO_PLAY, (long)buffer, (long)n);
    }
    printf("Hecho.\n");
    free(buffer);
    fclose(f);
}
