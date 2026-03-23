#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02

struct vfs_node;

typedef struct {
    uint32_t (*read)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
    uint32_t (*write)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
    struct vfs_node *(*finddir)(struct vfs_node *node, const char *name);
} vfs_ops_t;

typedef struct vfs_node {
    char name[128];
    uint32_t type;
    uint32_t size;
    vfs_ops_t *ops;
    void *priv_data; // Datos específicos del sistema de archivos
} vfs_node_t;

/* El nodo raíz del sistema */
extern vfs_node_t *vfs_root;

/* Inicializa el sistema de archivos virtual */
void vfs_init(void);

/* Funciones de conveniencia */
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_node_t *vfs_open(const char *path);

#endif
