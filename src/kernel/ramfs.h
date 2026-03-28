#ifndef RAMFS_H
#define RAMFS_H

#include "vfs.h"

/* Inicializa un sistema de archivos RAM escribible */
vfs_node_t *ramfs_init(void);

/* Crea un nuevo archivo en el RamFS */
int ramfs_create(const char *name, uint32_t size);

#endif
