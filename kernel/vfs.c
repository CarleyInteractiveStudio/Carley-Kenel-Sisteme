#include "vfs.h"
#include "kheap.h"
#include "common/string.h"

vfs_node_t *vfs_root = NULL;

void vfs_init(void) {}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->ops && node->ops->read) {
        return node->ops->read(node, offset, size, buffer);
    }
    return 0;
}

vfs_node_t *vfs_open(const char *path) {
    if (!vfs_root || !path) return NULL;
    if (path[0] == '/') path++;
    if (strlen(path) == 0) return vfs_root;

    if (vfs_root->ops && vfs_root->ops->finddir) {
        return vfs_root->ops->finddir(vfs_root, path);
    }
    return NULL;
}

int vfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent) {
    if (node && node->ops && node->ops->readdir) {
        return node->ops->readdir(node, index, dirent);
    }
    return -1;
}
