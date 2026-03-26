#ifndef LIB_BMP_H
#define LIB_BMP_H

#include <stdint.h>
#include <stdio.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} bmp_file_header_t;

typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    int32_t  x_pels_per_meter;
    int32_t  y_pels_per_meter;
    uint32_t clr_used;
    uint32_t clr_important;
} bmp_info_header_t;
#pragma pack(pop)

uint32_t *bmp_load(const char *path, uint32_t *w, uint32_t *h);

#endif
