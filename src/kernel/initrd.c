#include "initrd.h"
#include "kheap.h"
#include "string.h"
#include "limine.h"
#include "drivers/ide.h"

#define MAX_INITRD_FILES 32

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

typedef struct {
    char name[64];
    uint32_t size;
} pack_header_t;

void initrd_load_custom(void) {
    // Leer el sector 20480 (10MB) donde pusimos el initrd.bin en el Makefile
    uint8_t *buffer = kmalloc(512);
    ide_read_sectors(20480, 1, buffer);

    uint32_t file_count = *(uint32_t *)buffer;
    uint32_t current_sector = 20480;
    uint32_t offset_in_sector = 4;

    for (uint32_t i = 0; i < file_count; i++) {
        pack_header_t header;

        // Leer cabecera (68 bytes)
        if (offset_in_sector + sizeof(pack_header_t) > 512) {
            current_sector++;
            ide_read_sectors(current_sector, 1, buffer);
            offset_in_sector = 0;
        }
        memcpy(&header, buffer + offset_in_sector, sizeof(pack_header_t));
        offset_in_sector += sizeof(pack_header_t);

        vfs_node_t *file_node = kmalloc(sizeof(vfs_node_t));
        strcpy(file_node->name, header.name);
        file_node->type = VFS_FILE;
        file_node->size = header.size;

        void *file_data = kmalloc(header.size);
        uint32_t bytes_loaded = 0;

        while (bytes_loaded < header.size) {
            uint32_t to_copy = 512 - offset_in_sector;
            if (to_copy > (header.size - bytes_loaded)) to_copy = header.size - bytes_loaded;

            memcpy((uint8_t*)file_data + bytes_loaded, buffer + offset_in_sector, to_copy);
            bytes_loaded += to_copy;
            offset_in_sector += to_copy;

            if (offset_in_sector >= 512 && bytes_loaded < header.size) {
                current_sector++;
                ide_read_sectors(current_sector, 1, buffer);
                offset_in_sector = 0;
            }
        }
        file_node->priv_data = file_data;

        static vfs_ops_t file_ops = {.read = initrd_read_ops, .finddir = NULL, .readdir = NULL};
        file_node->ops = &file_ops;
        initrd_files[initrd_file_count++] = file_node;
    }

    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "/");
    root->type = VFS_DIRECTORY;
    root->size = 0;
    static vfs_ops_t root_ops = {.read = NULL, .finddir = initrd_finddir_ops, .readdir = initrd_readdir_ops};
    root->ops = &root_ops;
    vfs_root = root;
}
