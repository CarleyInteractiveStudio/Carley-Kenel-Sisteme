#include "carleyfs.h"
#include "drivers/ide.h"
#include "kheap.h"
#include "common/string.h"

#define CARLEYFS_MAX_FILES 64

static carleyfs_inode_t inodes[CARLEYFS_MAX_FILES];
static bool inodes_loaded = false;

static void cfs_load_inodes(void) {
    if (inodes_loaded) return;
    /* Leer 32 sectores (16KB) que contienen la tabla de inodos (Sectores 2-33) */
    ide_read_sectors(2, 32, (uint8_t *)inodes);
    inodes_loaded = true;
}

static uint32_t cfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    carleyfs_inode_t *inode = (carleyfs_inode_t *)node->priv_data;
    if (offset >= inode->size) return 0;
    if (offset + size > inode->size) size = inode->size - offset;

    uint32_t start_lba = inode->start_sector + (offset / 512);
    uint32_t sector_offset = offset % 512;
    uint8_t temp_buf[512];

    uint32_t written_bytes = 0;
    while (written_bytes < size) {
        ide_read_sectors(start_lba, 1, temp_buf);
        uint32_t to_copy = 512 - sector_offset;
        if (to_copy > size - written_bytes) to_copy = size - written_bytes;

        memcpy(temp_buf + sector_offset, buffer + written_bytes, to_copy);
        ide_write_sectors(start_lba++, 1, temp_buf);

        written_bytes += to_copy;
        sector_offset = 0;
    }
    return size;
}

static uint32_t cfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    carleyfs_inode_t *inode = (carleyfs_inode_t *)node->priv_data;
    if (offset >= inode->size) return 0;
    if (offset + size > inode->size) size = inode->size - offset;

    uint32_t start_lba = inode->start_sector + (offset / 512);
    uint32_t sector_offset = offset % 512;
    uint8_t temp_buf[512];

    uint32_t read_bytes = 0;
    while (read_bytes < size) {
        ide_read_sectors(start_lba++, 1, temp_buf);
        uint32_t to_copy = 512 - sector_offset;
        if (to_copy > size - read_bytes) to_copy = size - read_bytes;

        memcpy(buffer + read_bytes, temp_buf + sector_offset, to_copy);
        read_bytes += to_copy;
        sector_offset = 0;
    }
    return size;
}

static int cfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent) {
    (void)node;
    cfs_load_inodes();

    uint32_t current = 0;
    for (int i = 0; i < CARLEYFS_MAX_FILES; i++) {
        if (inodes[i].used) {
            if (current == index) {
                strcpy(dirent->name, inodes[i].name);
                dirent->size = inodes[i].size;
                dirent->type = VFS_FILE;
                return 0;
            }
            current++;
        }
    }
    return -1;
}

static vfs_node_t *cfs_finddir(vfs_node_t *node, const char *name) {
    (void)node;
    cfs_load_inodes();
    for (int i = 0; i < CARLEYFS_MAX_FILES; i++) {
        if (inodes[i].used && strcmp(inodes[i].name, name) == 0) {
            vfs_node_t *fn = kmalloc(sizeof(vfs_node_t));
            strcpy(fn->name, inodes[i].name);
            fn->size = inodes[i].size;
            fn->type = VFS_FILE;
            fn->priv_data = &inodes[i];
            static vfs_ops_t ops = {.read = cfs_read, .write = cfs_write, .finddir = NULL, .readdir = NULL};
            fn->ops = &ops;
            return fn;
        }
    }
    return NULL;
}

vfs_node_t *carleyfs_init(void) {
    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "disk");
    root->type = VFS_DIRECTORY;
    static vfs_ops_t root_ops = {.read = NULL, .write = NULL, .finddir = cfs_finddir, .readdir = cfs_readdir};
    root->ops = &root_ops;
    return root;
}
