#include "vfs.h"
#include "kheap.h"
#include "common/string.h"

vfs_node_t *vfs_root = NULL;
static vfs_node_t *mount_points[8];
static int mount_count = 0;

void vfs_init(void) {}

void vfs_mount(vfs_node_t *node) {
    if (mount_count < 8) mount_points[mount_count++] = node;
}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->ops && node->ops->read) return node->ops->read(node, offset, size, buffer);
    return 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->ops && node->ops->write) return node->ops->write(node, offset, size, buffer);
    return 0;
}

vfs_node_t *vfs_open(const char *path) {
    if (!vfs_root || !path) return NULL;

    /* Si es la raiz */
    if (strcmp(path, "/") == 0) return vfs_root;

    if (path[0] == '/') path++;

    char first_part[128];
    int i = 0;
    while (path[i] && path[i] != '/') {
        first_part[i] = path[i];
        i++;
    }
    first_part[i] = 0;

    /* 1. Buscar en los puntos de montaje */
    for (int j = 0; j < mount_count; j++) {
        if (strcmp(mount_points[j]->name, first_part) == 0) {
            if (path[i] == '/') {
                if (mount_points[j]->ops && mount_points[j]->ops->finddir) {
                    return mount_points[j]->ops->finddir(mount_points[j], path + i + 1);
                }
            }
            return mount_points[j];
        }
    }

    /* 2. Buscar recursivamente en el root */
    if (vfs_root->ops && vfs_root->ops->finddir) {
        return vfs_root->ops->finddir(vfs_root, path);
    }

    return NULL;
}

int vfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent) {
    if (node && node->ops && node->ops->readdir) return node->ops->readdir(node, index, dirent);
    return -1;
}
