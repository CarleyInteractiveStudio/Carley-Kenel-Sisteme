#ifndef CARLEYFS_H
#define CARLEYFS_H

#include "vfs.h"

/* Estructura simple para CarleyFS:
   - Sector 1: Superbloque (Num archivos, etc.)
   - Sectores 2-33: Nodos de archivo (Nombre, tamaño, sector de inicio)
   - Sectores 34+: Datos de archivos
*/

typedef struct {
    char name[64];
    uint32_t size;
    uint32_t start_sector;
    uint32_t used;
} carleyfs_inode_t;

vfs_node_t *carleyfs_init(void);

#endif
