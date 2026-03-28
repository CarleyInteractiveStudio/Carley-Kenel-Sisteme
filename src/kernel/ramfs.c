#include "ramfs.h"
#include "kheap.h"
#include "string.h"

#define MAX_RAMFS_FILES 32

static vfs_node_t *ram_files[MAX_RAMFS_FILES];
static int ram_file_count = 0;

static uint32_t ramfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (offset >= node->size) return 0;
    if (offset + size > node->size) size = node->size - offset;
    memcpy(buffer, (uint8_t *)node->priv_data + offset, size);
    return size;
}

static uint32_t ramfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    /* Por simplicidad, el tamaño es fijo al crear el archivo */
    if (offset >= node->size) return 0;
    if (offset + size > node->size) size = node->size - offset;
    memcpy((uint8_t *)node->priv_data + offset, buffer, size);
    return size;
}

static int ramfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent) {
    (void)node;
    if (index < (uint32_t)ram_file_count) {
        strcpy(dirent->name, ram_files[index]->name);
        dirent->size = ram_files[index]->size;
        dirent->type = VFS_FILE;
        return 0;
    }
    return -1;
}

static vfs_node_t *ramfs_finddir(vfs_node_t *node, const char *name) {
    (void)node;
    for (int i = 0; i < ram_file_count; i++) {
        if (strcmp(ram_files[i]->name, name) == 0) return ram_files[i];
    }
    return NULL;
}

int ramfs_create(const char *name, uint32_t size) {
    if (ram_file_count >= MAX_RAMFS_FILES) return -1;

    vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
    strcpy(node->name, name);
    node->size = size;
    node->type = VFS_FILE;
    node->priv_data = kmalloc(size);
    memset(node->priv_data, 0, size);

    static vfs_ops_t ops = {.read = ramfs_read, .write = ramfs_write, .finddir = NULL, .readdir = NULL};
    node->ops = &ops;

    ram_files[ram_file_count++] = node;
    return 0;
}

static int ramfs_create_wrapper(vfs_node_t *node, const char *name, uint32_t size) {
    (void)node;
    return ramfs_create(name, size);
}

vfs_node_t *ramfs_init(void) {
    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "ram");
    root->type = VFS_DIRECTORY;
    static vfs_ops_t root_ops = {
        .read = NULL, .write = NULL,
        .finddir = ramfs_finddir, .readdir = ramfs_readdir,
        .create = ramfs_create_wrapper, .mkdir = NULL
    };
    root->ops = &root_ops;
    return root;
}
