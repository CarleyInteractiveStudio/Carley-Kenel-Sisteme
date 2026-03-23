#include "vfs.h"
#include "kheap.h"
#include "common/string.h"

vfs_node_t *vfs_root = NULL;

void vfs_init(void) {
    /* La raíz se inicializará con el punto de montaje del initrd */
}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->ops && node->ops->read) {
        return node->ops->read(node, offset, size, buffer);
    }
    return 0;
}

/* Muy simplificado por ahora: solo busca en la raíz */
vfs_node_t *vfs_open(const char *path) {
    if (!vfs_root || !path) return NULL;

    /* Si el path empieza con /, saltarlo */
    if (path[0] == '/') path++;

    if (vfs_root->ops && vfs_root->ops->finddir) {
        return vfs_root->ops->finddir(vfs_root, path);
    }
    return NULL;
}
