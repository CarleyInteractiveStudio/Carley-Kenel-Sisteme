#include "initrd.h"
#include "kheap.h"
#include "common/string.h"
#include "common/limine.h"

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

void initrd_load_all(struct limine_module_response *response) {
    if (!response) return;

    for (uint64_t i = 0; i < response->module_count; i++) {
        struct limine_file *module = response->modules[i];

        vfs_node_t *file_node = kmalloc(sizeof(vfs_node_t));

        const char *filename = module->path;
        const char *slash = filename;
        while (*filename) {
            if (*filename == '/') slash = filename + 1;
            filename++;
        }

        strcpy(file_node->name, slash);
        file_node->type = VFS_FILE;
        file_node->size = module->size;
        file_node->priv_data = module->address;

        static vfs_ops_t file_ops = {.read = initrd_read_ops, .finddir = NULL, .readdir = NULL};
        file_node->ops = &file_ops;

        if (initrd_file_count < MAX_INITRD_FILES) {
            initrd_files[initrd_file_count++] = file_node;
        }
    }

    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "/");
    root->type = VFS_DIRECTORY;
    root->size = 0;
    static vfs_ops_t root_ops = {.read = NULL, .finddir = initrd_finddir_ops, .readdir = initrd_readdir_ops};
    root->ops = &root_ops;
    vfs_root = root;
}
