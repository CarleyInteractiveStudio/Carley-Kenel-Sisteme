#include "carleyfs.h"
#include "drivers/ide.h"
#include "kheap.h"
#include "string.h"

#define CARLEYFS_MAX_FILES 64

static carleyfs_inode_t inodes[CARLEYFS_MAX_FILES];
static carleyfs_superblock_t superblock;
static bool inodes_loaded = false;

static void cfs_load_metadata(void) {
    if (inodes_loaded) return;
    /* Sector 1: Superbloque */
    ide_read_sectors(1, 1, (uint8_t *)&superblock);
    if (superblock.magic != 0xCA121E1) {
        superblock.magic = 0xCA121E1;
        superblock.num_inodes = 0;
        superblock.next_free_sector = 34; // Empieza después de los inodos
    }
    /* Leer 32 sectores (16KB) que contienen la tabla de inodos (Sectores 2-33) */
    ide_read_sectors(2, 32, (uint8_t *)inodes);
    inodes_loaded = true;
}

static void cfs_save_metadata(void) {
    ide_write_sectors(1, 1, (uint8_t *)&superblock);
    ide_write_sectors(2, 32, (uint8_t *)inodes);
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
    cfs_load_metadata();

    uint32_t current = 0;
    for (int i = 0; i < CARLEYFS_MAX_FILES; i++) {
        if (inodes[i].used) {
            if (current == index) {
                strcpy(dirent->name, inodes[i].name);
                dirent->size = inodes[i].size;
                dirent->type = inodes[i].type == 2 ? VFS_DIRECTORY : VFS_FILE;
                return 0;
            }
            current++;
        }
    }
    return -1;
}

static vfs_node_t *cfs_finddir(vfs_node_t *node, const char *name) {
    (void)node;
    cfs_load_metadata();
    for (int i = 0; i < CARLEYFS_MAX_FILES; i++) {
        if (inodes[i].used && strcmp(inodes[i].name, name) == 0) {
            vfs_node_t *fn = kmalloc(sizeof(vfs_node_t));
            strcpy(fn->name, inodes[i].name);
            fn->size = inodes[i].size;
            fn->type = inodes[i].type == 2 ? VFS_DIRECTORY : VFS_FILE;
            fn->priv_data = &inodes[i];
            static vfs_ops_t ops = {.read = cfs_read, .write = cfs_write, .finddir = NULL, .readdir = NULL};
            fn->ops = &ops;
            return fn;
        }
    }
    return NULL;
}

static int cfs_create(vfs_node_t *node, const char *name, uint32_t size) {
    (void)node;
    cfs_load_metadata();

    for (int i = 0; i < CARLEYFS_MAX_FILES; i++) {
        if (!inodes[i].used) {
            strncpy(inodes[i].name, name, 63);
            inodes[i].name[63] = 0;
            inodes[i].size = size;
            inodes[i].type = 1; // File
            inodes[i].used = 1;
            inodes[i].start_sector = superblock.next_free_sector;

            uint32_t sectors = (size + 511) / 512;
            superblock.next_free_sector += sectors;
            superblock.num_inodes++;

            cfs_save_metadata();
            return 0;
        }
    }
    return -1;
}

static int cfs_mkdir(vfs_node_t *node, const char *name) {
    (void)node;
    cfs_load_metadata();
    for (int i = 0; i < CARLEYFS_MAX_FILES; i++) {
        if (!inodes[i].used) {
            strncpy(inodes[i].name, name, 63);
            inodes[i].name[63] = 0;
            inodes[i].size = 0;
            inodes[i].type = 2; // Directory
            inodes[i].used = 1;
            inodes[i].start_sector = 0;
            superblock.num_inodes++;
            cfs_save_metadata();
            return 0;
        }
    }
    return -1;
}

vfs_node_t *carleyfs_init(void) {
    vfs_node_t *root = kmalloc(sizeof(vfs_node_t));
    strcpy(root->name, "disk");
    root->type = VFS_DIRECTORY;
    static vfs_ops_t root_ops = {
        .read = NULL, .write = NULL,
        .finddir = cfs_finddir, .readdir = cfs_readdir,
        .create = cfs_create, .mkdir = cfs_mkdir
    };
    root->ops = &root_ops;
    return root;
}
