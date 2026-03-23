#ifndef INITRD_H
#define INITRD_H

#include <stdint.h>
#include "common/limine.h"
#include "vfs.h"

/* Estructura simple de cabecera para archivos en el Initrd */
typedef struct {
    char name[64];
    uint32_t size;
    uint32_t offset; // Offset desde el inicio del modulo
} initrd_file_header_t;

/* Inicializa el sistema de archivos del Initrd basado en un modulo de Limine */
vfs_node_t *initrd_init(void *addr, uint64_t size);

#endif
