#include "initrd.h"
#include "kheap.h"
#include "common/string.h"

#define MAX_INITRD_FILES 16

static vfs_node_t *initrd_files[MAX_INITRD_FILES];
static int initrd_file_count = 0;

static uint32_t initrd_read_ops(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (offset >= node->size) return 0;
    if (offset + size > node->size) size = node->size - offset;
    memcpy(buffer, (uint8_t *)node->priv_data + offset, size);
    return size;
}

static int initrd_readdir_ops(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent) {
    (void)node;
    if (index < (uint32_t)initrd_file_count) {
        strcpy(dirent->name, initrd_files[index]->name);
        dirent->size = initrd_files[index]->size;
        dirent->type = VFS_FILE;
        return 0;
    }
    return -1;
}

static vfs_node_t *initrd_finddir_ops(vfs_node_t *node, const char *name) {
    (void)node;
    for (int i = 0; i < initrd_file_count; i++) {
        if (strcmp(initrd_files[i]->name, name) == 0) {
            return initrd_files[i];
        }
    }
    return NULL;
}

vfs_node_t *initrd_init(void *addr, uint64_t size) {
    /* Por ahora, tratamos el modulo unico como un archivo llamado 'shell.elf' */
    /* En un sistema real, parseariamos una estructura tar o cpio */

    vfs_node_t *file_node = kmalloc(sizeof(vfs_node_t));
    strcpy(file_node->name, "shell.elf");
    file_node->type = VFS_FILE;
    file_node->size = size;
    file_node->priv_data = addr;
    static vfs_ops_t file_ops = {.read = initrd_read_ops, .finddir = NULL, .readdir = NULL};
    file_node->ops = &file_ops;

    initrd_files[0] = file_node;
    initrd_file_count = 1;

    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "/");
    root->type = VFS_DIRECTORY;
    root->size = 0;

    static vfs_ops_t root_ops = {
        .read = NULL,
        .finddir = initrd_finddir_ops,
        .readdir = initrd_readdir_ops
    };
    root->ops = &root_ops;

    return root;
}
