#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INODES 64
#define SUPERBLOCK_SECTOR 64
#define INODE_SECTOR_START 65
#define DATA_SECTOR_START 96

typedef struct {
    char name[64];
    uint32_t size;
    uint32_t start_sector;
    uint32_t type; // 1 = File, 2 = Directory
    uint32_t used;
} carleyfs_inode_t;

typedef struct {
    uint32_t magic;
    uint32_t num_inodes;
    uint32_t next_free_sector;
    uint32_t reserved[125];
} carleyfs_superblock_t;

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Uso: %s <imagen.img> <bootloader.bin> <kernel.bin> [initrd.bin]\n", argv[0]);
        return 1;
    }

    FILE *img = fopen(argv[1], "wb");
    if (!img) return 1;

    // 1. Escribir ceros (40MB)
    uint8_t *zero = (uint8_t *)calloc(1, 1024 * 1024);
    for (int i = 0; i < 40; i++) fwrite(zero, 1, 1024 * 1024, img);
    free(zero);

    // 2. Escribir Bootloader (LBA 0)
    FILE *boot = fopen(argv[2], "rb");
    fseek(boot, 0, SEEK_END);
    size_t boot_size = ftell(boot);
    rewind(boot);
    uint8_t *boot_buf = (uint8_t *)malloc(boot_size);
    fread(boot_buf, 1, boot_size, boot);
    fseek(img, 0, SEEK_SET);
    fwrite(boot_buf, 1, boot_size, img);
    fclose(boot);

    // 3. Crear Estructura CarleyFS
    carleyfs_superblock_t sb = {0};
    sb.magic = 0xCA121E1;
    sb.num_inodes = 0;
    sb.next_free_sector = DATA_SECTOR_START;

    carleyfs_inode_t inodes[MAX_INODES] = {0};

    // Añadir Kernel (LBA 2048 para compatibilidad o según FS)
    // Para simplificar el cargador, lo pondremos en un LBA fijo o buscaremos.
    // Lo pondremos en DATA_SECTOR_START.

    auto add_file = [&](const char *path, const char *name) {
        FILE *f = fopen(path, "rb");
        if (!f) return;
        fseek(f, 0, SEEK_END);
        uint32_t size = ftell(f);
        rewind(f);

        uint8_t *buf = (uint8_t *)malloc(size);
        fread(buf, 1, size, f);

        uint32_t start = sb.next_free_sector;
        fseek(img, start * 512, SEEK_SET);
        fwrite(buf, 1, size, img);

        strncpy(inodes[sb.num_inodes].name, name, 63);
        inodes[sb.num_inodes].size = size;
        inodes[sb.num_inodes].start_sector = start;
        inodes[sb.num_inodes].type = 1;
        inodes[sb.num_inodes].used = 1;

        sb.num_inodes++;
        sb.next_free_sector += (size + 511) / 512;

        free(buf);
        fclose(f);
    };

    add_file(argv[3], "kernel");
    if (argc > 4) add_file(argv[4], "initrd");
    if (argc > 5) add_file(argv[5], "ap_trampoline");

    // Escribir Superbloque (LBA 64)
    fseek(img, SUPERBLOCK_SECTOR * 512, SEEK_SET);
    fwrite(&sb, 1, 512, img);

    // Escribir Tabla de Inodos (LBA 65)
    fseek(img, 512 * INODE_SECTOR_START, SEEK_SET);
    fwrite(inodes, sizeof(carleyfs_inode_t), MAX_INODES, img);

    fclose(img);
    printf("CarleyFS: Imagen generada con %u archivos.\n", sb.num_inodes);
    return 0;
}
