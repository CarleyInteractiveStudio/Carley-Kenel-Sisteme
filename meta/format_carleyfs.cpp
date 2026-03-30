#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>

/* Synchronized with common/config.h */
#define MAX_INODES 128
#define SUPERBLOCK_SECTOR 128
#define INODE_SECTOR_START 132
#define DATA_SECTOR_START 256
#define INITRD_SECTOR 20480

typedef struct {
    char name[64];
    uint32_t size;
    uint32_t start_sector;
    uint32_t type;
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
        std::cerr << "Uso: " << argv[0] << " <imagen.img> <bootloader.bin> <kernel.bin> [initrd.bin] [ap_trampoline.bin]" << std::endl;
        return 1;
    }

    std::ofstream img(argv[1], std::ios::binary);
    if (!img) return 1;

    // 1. Escribir ceros (40MB)
    std::vector<char> zero(1024 * 1024, 0);
    for (int i = 0; i < 40; i++) img.write(zero.data(), zero.size());

    // 2. Escribir Bootloader (LBA 0)
    std::ifstream boot(argv[2], std::ios::binary);
    if (boot) {
        std::vector<char> boot_buf((std::istreambuf_iterator<char>(boot)), std::istreambuf_iterator<char>());
        img.seekp(0, std::ios::beg);
        img.write(boot_buf.data(), boot_buf.size());
    }

    // 3. Crear Estructura CarleyFS
    carleyfs_superblock_t sb = {0};
    sb.magic = 0xCA121E1;
    sb.num_inodes = 0;
    sb.next_free_sector = DATA_SECTOR_START;

    std::vector<carleyfs_inode_t> inodes(MAX_INODES, {0});

    auto add_file = [&](const char *path, const char *name) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return;
        std::vector<char> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

        uint32_t start = sb.next_free_sector;
        if (std::strcmp(name, "initrd") == 0) {
            start = INITRD_SECTOR;
        }

        // ALINEACIÓN CRÍTICA: Forzamos alineación a 2048 bytes (4 sectores lógicos)
        // Esto asegura que cada archivo empiece al inicio de un sector físico de CD.
        if (start != INITRD_SECTOR) {
            start = (start + 3) & ~3;
        }

        img.seekp(start * 512, std::ios::beg);
        img.write(buf.data(), buf.size());

        std::strncpy(inodes[sb.num_inodes].name, name, 63);
        inodes[sb.num_inodes].size = (uint32_t)buf.size();
        inodes[sb.num_inodes].start_sector = start;
        inodes[sb.num_inodes].type = 1;
        inodes[sb.num_inodes].used = 1;

        sb.num_inodes++;
        if (start != INITRD_SECTOR) {
            sb.next_free_sector = start + ((uint32_t)buf.size() + 511) / 512;
        }
    };

    add_file(argv[3], "kernel");
    if (argc > 4) add_file(argv[4], "initrd");
    if (argc > 5) add_file(argv[5], "ap_trampoline");

    // Escribir Superbloque (LBA 128)
    img.seekp(SUPERBLOCK_SECTOR * 512, std::ios::beg);
    img.write(reinterpret_cast<char*>(&sb), sizeof(sb));

    // Escribir Tabla de Inodos (LBA 132)
    img.seekp(INODE_SECTOR_START * 512, std::ios::beg);
    img.write(reinterpret_cast<char*>(inodes.data()), inodes.size() * sizeof(carleyfs_inode_t));

    img.close();
    std::cout << "CarleyFS: Imagen generada con " << (int)sb.num_inodes << " archivos y alineación 2048-bytes." << std::endl;
    return 0;
}
