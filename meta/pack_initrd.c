#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    char name[64];
    uint32_t size;
} file_header_t;

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Uso: %s <output> <file1> <file2> ...\n", argv[0]);
        return 1;
    }

    FILE *out = fopen(argv[1], "wb");
    if (!out) return 1;

    uint32_t file_count = argc - 2;
    fwrite(&file_count, 4, 1, out);

    for (int i = 2; i < argc; i++) {
        FILE *in = fopen(argv[i], "rb");
        if (!in) continue;

        fseek(in, 0, SEEK_END);
        uint32_t size = ftell(in);
        fseek(in, 0, SEEK_SET);

        file_header_t header;
        memset(&header, 0, sizeof(header));

        // Solo el nombre del archivo, no la ruta completa
        char *base_name = strrchr(argv[i], '/');
        if (base_name) base_name++;
        else base_name = argv[i];

        strncpy(header.name, base_name, 63);
        header.size = size;

        fwrite(&header, sizeof(header), 1, out);

        uint8_t *buffer = malloc(size);
        fread(buffer, size, 1, in);
        fwrite(buffer, size, 1, out);
        free(buffer);

        fclose(in);
    }

    fclose(out);
    return 0;
}
