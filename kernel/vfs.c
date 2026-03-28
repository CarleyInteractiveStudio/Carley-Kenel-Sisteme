#include "vfs.h"
#include "kheap.h"
#include "common/string.h"
#include "spinlock.h"

vfs_node_t *vfs_root = NULL;
static vfs_node_t *mount_points[8];
static int mount_count = 0;
static spinlock_t vfs_lock = 0;

void vfs_init(void) {}

void vfs_mount(vfs_node_t *node) {
    spin_lock(&vfs_lock);
    if (mount_count < 8) mount_points[mount_count++] = node;
    spin_unlock(&vfs_lock);
}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->ops && node->ops->read) return node->ops->read(node, offset, size, buffer);
    return 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->ops && node->ops->write) return node->ops->write(node, offset, size, buffer);
    return 0;
}

vfs_node_t *vfs_open(const char *path) {
    spin_lock(&vfs_lock);
    if (!vfs_root || !path) { spin_unlock(&vfs_lock); return NULL; }

    if (strcmp(path, "/") == 0) { spin_unlock(&vfs_lock); return vfs_root; }

    const char *p = path;
    if (p[0] == '/') p++;

    char part[128];
    vfs_node_t *curr = vfs_root;

    // 1. Verificar si empieza por un punto de montaje
    int i = 0;
    while (p[i] && p[i] != '/' && i < 127) { part[i] = p[i]; i++; }
    part[i] = 0;

    for (int j = 0; j < mount_count; j++) {
        if (strcmp(mount_points[j]->name, part) == 0) {
            curr = mount_points[j];
            p += i;
            if (p[0] == '/') p++;
            break;
        }
    }

    // 2. Navegación recursiva/secuencial
    while (*p) {
        i = 0;
        while (p[i] && p[i] != '/' && i < 127) { part[i] = p[i]; i++; }
        part[i] = 0;

        if (strcmp(part, ".") == 0) { /* Ignorar */ }
        else if (strcmp(part, "..") == 0) {
            if (curr->parent) curr = curr->parent;
        } else if (curr->ops && curr->ops->finddir) {
            vfs_node_t *next = curr->ops->finddir(curr, part);
            if (!next) { spin_unlock(&vfs_lock); return NULL; }
            next->parent = curr; // Establecer padre dinámicamente
            curr = next;
        } else {
            spin_unlock(&vfs_lock); return NULL;
        }

        p += i;
        if (p[0] == '/') p++;
    }

    spin_unlock(&vfs_lock);
    return curr;
}

int vfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent) {
    if (node && node->ops && node->ops->readdir) return node->ops->readdir(node, index, dirent);
    return -1;
}

int vfs_create(const char *path, uint32_t size) {
    spin_lock(&vfs_lock);
    // Para simplificar, buscamos si el path empieza por el nombre de un punto de montaje
    if (path[0] == '/') path++;

    for (int i = 0; i < mount_count; i++) {
        size_t mlen = strlen(mount_points[i]->name);
        if (strncmp(path, mount_points[i]->name, mlen) == 0 && path[mlen] == '/') {
            if (mount_points[i]->ops->create) {
                int res = mount_points[i]->ops->create(mount_points[i], path + mlen + 1, size);
                spin_unlock(&vfs_lock);
                return res;
            }
        }
    }
    spin_unlock(&vfs_lock);
    return -1;
}

int vfs_mkdir(const char *path) {
    spin_lock(&vfs_lock);
    if (path[0] == '/') path++;

    for (int i = 0; i < mount_count; i++) {
        size_t mlen = strlen(mount_points[i]->name);
        if (strncmp(path, mount_points[i]->name, mlen) == 0 && path[mlen] == '/') {
            if (mount_points[i]->ops->mkdir) {
                int res = mount_points[i]->ops->mkdir(mount_points[i], path + mlen + 1);
                spin_unlock(&vfs_lock);
                return res;
            }
        }
    }
    spin_unlock(&vfs_lock);
    return -1;
}
