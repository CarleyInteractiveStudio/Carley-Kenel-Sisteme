#include "carleyfs.h"
#include "drivers/ide.h"
#include "kheap.h"
#include "common/string.h"

#define CARLEYFS_MAX_FILES 64

static carleyfs_inode_t inodes[CARLEYFS_MAX_FILES];

static uint32_t cfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    carleyfs_inode_t *inode = (carleyfs_inode_t *)node->priv_data;
    if (offset >= inode->size) return 0;
    if (offset + size > inode->size) size = inode->size - offset;

    /* Leer sectores del disco real */
    uint32_t start_lba = inode->start_sector + (offset / 512);
    uint32_t sector_offset = offset % 512;
    uint8_t temp_buf[512];

    uint32_t read = 0;
    while (read < size) {
        ide_read_sectors(start_lba++, 1, temp_buf);
        uint32_t to_copy = 512 - sector_offset;
        if (to_copy > size - read) to_copy = size - read;

        memcpy(buffer + read, temp_buf + sector_offset, to_copy);
        read += to_copy;
        sector_offset = 0;
    }
    return size;
}

static vfs_node_t *cfs_finddir(vfs_node_t *node, const char *name) {
    (void)node;
    for (int i = 0; i < CARLEYFS_MAX_FILES; i++) {
        if (inodes[i].used && strcmp(inodes[i].name, name) == 0) {
            vfs_node_t *fn = kmalloc(sizeof(vfs_node_t));
            strcpy(fn->name, inodes[i].name);
            fn->size = inodes[i].size;
            fn->type = VFS_FILE;
            fn->priv_data = &inodes[i];
            static vfs_ops_t ops = {.read = cfs_read, .write = NULL, .finddir = NULL, .readdir = NULL};
            fn->ops = &ops;
            return fn;
        }
    }
    return NULL;
}

vfs_node_t *carleyfs_init(void) {
    /* Leer tabla de inodos del disco (Sectores 2-33) */
    /* Por simplicidad en la demo, asumimos que ya existen algunos datos en el disco */
    /* ide_read_sectors(2, 32, (uint8_t*)inodes); */

    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "disk");
    root->type = VFS_DIRECTORY;
    static vfs_ops_t root_ops = {.read = NULL, .write = NULL, .finddir = cfs_finddir, .readdir = NULL};
    root->ops = &root_ops;
    return root;
}
