#include "initrd.h"
#include "kheap.h"
#include "common/string.h"

static void *initrd_base = NULL;
static uint64_t initrd_size = 0;

/* Implementación de las operaciones del Initrd */
static uint32_t initrd_read_ops(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    initrd_file_header_t *header = (initrd_file_header_t *)node->priv_data;

    if (offset >= header->size) return 0;
    if (offset + size > header->size) size = header->size - offset;

    memcpy(buffer, (uint8_t *)initrd_base + header->offset + offset, size);
    return size;
}

static vfs_node_t *initrd_finddir_ops(vfs_node_t *node, const char *name) {
    (void)node;
    uint8_t *ptr = (uint8_t *)initrd_base;
    uint32_t n_files = *(uint32_t *)ptr;
    ptr += 4;

    for (uint32_t i = 0; i < n_files; i++) {
        initrd_file_header_t *header = (initrd_file_header_t *)ptr;
        if (strcmp(header->name, name) == 0) {
            vfs_node_t *file_node = kmalloc(sizeof(vfs_node_t));
            strcpy(file_node->name, header->name);
            file_node->size = header->size;
            file_node->type = VFS_FILE;
            file_node->priv_data = header;

            static vfs_ops_t ops = {.read = initrd_read_ops, .finddir = NULL};
            file_node->ops = &ops;
            return file_node;
        }
        ptr += sizeof(initrd_file_header_t);
    }
    return NULL;
}

vfs_node_t *initrd_init(void *addr, uint64_t size) {
    initrd_base = addr;
    initrd_size = size;

    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "initrd_root");
    root->type = VFS_DIRECTORY;
    root->size = 0;

    static vfs_ops_t root_ops = {.read = NULL, .finddir = initrd_finddir_ops};
    root->ops = &root_ops;

    return root;
}
