#include "initrd.h"
#include "kheap.h"
#include "common/string.h"

/* Implementación del driver de modulo crudo para la demo */
static uint32_t raw_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (offset >= node->size) return 0;
    if (offset + size > node->size) size = node->size - offset;
    memcpy(buffer, (uint8_t *)node->priv_data + offset, size);
    return size;
}

vfs_node_t *initrd_init(void *addr, uint64_t size) {
    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "hello.elf");
    root->type = VFS_FILE;
    root->size = size;
    root->priv_data = addr;

    static vfs_ops_t ops = {.read = raw_read, .finddir = NULL};
    root->ops = &ops;
    return root;
}
