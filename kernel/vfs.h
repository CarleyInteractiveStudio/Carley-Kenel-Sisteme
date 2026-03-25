#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02

struct vfs_node;

typedef struct {
    char name[128];
    uint32_t size;
    uint32_t type;
} vfs_dirent_t;

typedef struct {
    uint32_t (*read)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
    uint32_t (*write)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
    struct vfs_node *(*finddir)(struct vfs_node *node, const char *name);
    int (*readdir)(struct vfs_node *node, uint32_t index, vfs_dirent_t *dirent);
    int (*create)(struct vfs_node *node, const char *name, uint32_t size);
    int (*mkdir)(struct vfs_node *node, const char *name);
} vfs_ops_t;

typedef struct vfs_node {
    char name[128];
    uint32_t type;
    uint32_t size;
    vfs_ops_t *ops;
    void *priv_data;
} vfs_node_t;

extern vfs_node_t *vfs_root;

void vfs_init(void);
void vfs_mount(vfs_node_t *node);
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
vfs_node_t *vfs_open(const char *path);
int vfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent);
int vfs_create(const char *path, uint32_t size);
int vfs_mkdir(const char *path);

#endif
