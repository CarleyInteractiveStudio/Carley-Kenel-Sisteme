#include "include/bmp.h"
#include "include/stdlib.h"

uint32_t *bmp_load(const char *path, uint32_t *w, uint32_t *h) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    bmp_file_header_t bfh;
    bmp_info_header_t bih;

    fread(&bfh, sizeof(bfh), 1, f);
    if (bfh.type != 0x4D42) { fclose(f); return NULL; }

    fread(&bih, sizeof(bih), 1, f);
    *w = bih.width;
    *h = (bih.height < 0) ? -bih.height : bih.height;

    uint32_t *data = (uint32_t *)malloc((*w) * (*h) * 4);
    fseek(f, bfh.offset, SEEK_SET);

    // Asumimos 32-bit o 24-bit para este prototipo
    if (bih.bit_count == 32) {
        fread(data, 1, (*w) * (*h) * 4, f);
    } else if (bih.bit_count == 24) {
        for (uint32_t i = 0; i < (*h); i++) {
            for (uint32_t j = 0; j < (*w); j++) {
                uint8_t bgr[3];
                fread(bgr, 1, 3, f);
                data[((*h) - 1 - i) * (*w) + j] = (bgr[2] << 16) | (bgr[1] << 8) | bgr[0] | 0xFF000000;
            }
        }
    }

    fclose(f);
    return data;
}
