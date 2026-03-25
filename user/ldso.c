#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

extern long syscall3(int num, long arg1, long arg2, long arg3);

/* ld-carley.so: Enlazador Dinamico del Sistema */

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
} Elf64_Ehdr;

typedef struct {
    int64_t d_tag;
    union {
        uint64_t d_val;
        uint64_t d_ptr;
    } d_un;
} Elf64_Dyn;

#define PT_DYNAMIC 2
#define DT_NULL    0
#define DT_NEEDED  1
#define DT_PLTGOT  3
#define DT_STRTAB  5
#define DT_SYMTAB  6
#define DT_RELA    7
#define DT_RELASZ  8
#define DT_RELAENT 9
#define DT_STRSZ   10
#define DT_SYMENT  11
#define DT_PLTRELSZ 2
#define DT_PLTREL  20
#define DT_JMPREL  23

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t  r_addend;
} Elf64_Rela;

typedef struct {
    uint32_t st_name;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)
#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_GLOB_DAT  6

extern void *mmap(void *addr, size_t length, int prot, int flags, int fd, uint32_t offset);
#define SYS_MMAP 14

static void *ld_load_library(const char *name) {
    FILE *f = fopen(name, "rb");
    if (!f) return NULL;

    // Obtenemos el tamaño (truco con vfs_node si FILE lo tiene expuesto,
    // pero LibC fopen nos da un stream. En este sistema FILE tiene vfs_node)
    // Para el prototipo, asumimos un tamaño o leemos el header.
    uint8_t header[64];
    fread(header, 1, 64, f);
    Elf64_Ehdr *eh = (Elf64_Ehdr *)header;

    // Mapear la librería.
    // En este sistema mmap(addr, len) simplemente reserva memoria.
    void *lib_base = (void *)0x800000; // Dirección fija para libc.so en el prototipo mejorado
    syscall3(SYS_MMAP, (long)lib_base, eh->e_phnum * 4096, 0); // Reserva simplificada

    // Cargar segmentos LOAD
    // (Simplificado: leemos todo el archivo a la base por ahora)
    fseek(f, 0, SEEK_SET);
    fread(lib_base, 1, 0x10000, f); // Leer hasta 64KB de la lib
    fclose(f);

    return lib_base;
}

static uint64_t find_symbol_in_lib(const char *name, void *lib_base) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)lib_base;
    Elf64_Phdr *phdrs = (Elf64_Phdr *)((uint8_t *)lib_base + ehdr->e_phoff);
    Elf64_Dyn *dyn = NULL;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn = (Elf64_Dyn *)((uint8_t *)lib_base + phdrs[i].p_vaddr);
            break;
        }
    }
    if (!dyn) return 0;

    Elf64_Sym *symtab = NULL;
    const char *strtab = NULL;
    for (; dyn->d_tag != DT_NULL; dyn++) {
        if (dyn->d_tag == DT_SYMTAB) symtab = (Elf64_Sym *)((uint8_t *)lib_base + dyn->d_un.d_ptr);
        if (dyn->d_tag == DT_STRTAB) strtab = (const char *)((uint8_t *)lib_base + dyn->d_un.d_ptr);
    }

    if (!symtab || !strtab) return 0;
    for (int i = 0; ; i++) {
        if (symtab[i].st_name == 0 && i > 0) break;
        if (strcmp(strtab + symtab[i].st_name, name) == 0) {
            return (uint64_t)lib_base + symtab[i].st_value;
        }
        if (i > 1000) break; // Seguridad
    }
    return 0;
}

void ld_main(int argc, char **argv, uint64_t app_entry) {
    printf("LdCarley: Iniciando enlazador para punto de entrada %p...\n", (void *)app_entry);

    /* En este prototipo, el kernel ya ha mapeado el binario y libc.so en memoria.
       El enlazador debe buscar la tabla DYNAMIC del ejecutable principal. */

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)0x400000; // Direccion base estandar
    Elf64_Phdr *phdrs = (Elf64_Phdr *)(0x400000 + ehdr->e_phoff);
    Elf64_Dyn *dyn = NULL;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn = (Elf64_Dyn *)phdrs[i].p_vaddr;
            break;
        }
    }

    Elf64_Sym *symtab = NULL;
    const char *strtab = NULL;
    Elf64_Rela *plt_rel = NULL;
    uint64_t plt_rel_sz = 0;

    if (dyn) {
        printf("LdCarley: Tabla DYNAMIC encontrada en %p.\n", (void *)dyn);
        for (Elf64_Dyn *d = dyn; d->d_tag != DT_NULL; d++) {
            if (d->d_tag == DT_SYMTAB) symtab = (Elf64_Sym *)d->d_un.d_ptr;
            if (d->d_tag == DT_STRTAB) strtab = (const char *)d->d_un.d_ptr;
            if (d->d_tag == DT_JMPREL) plt_rel = (Elf64_Rela *)d->d_un.d_ptr;
            if (d->d_tag == DT_PLTRELSZ) plt_rel_sz = d->d_un.d_val;
        }
    }

    /* Cargar dependencias (libc.so) */
    void *libc_base = ld_load_library("libc.so");
    if (!libc_base) {
        printf("LdCarley: ERROR - No se pudo cargar libc.so\n");
        return;
    }

    /* Resolucion de relas (PLT/GOT) */
    if (plt_rel && symtab && strtab) {
        int num_rel = plt_rel_sz / sizeof(Elf64_Rela);
        for (int i = 0; i < num_rel; i++) {
            uint64_t sym_idx = ELF64_R_SYM(plt_rel[i].r_info);
            const char *sym_name = strtab + symtab[sym_idx].st_name;

            uint64_t sym_addr = find_symbol_in_lib(sym_name, libc_base);
            if (sym_addr) {
                *(uint64_t *)plt_rel[i].r_offset = sym_addr;
                printf("LdCarley: Relocado %s -> %p\n", sym_name, (void *)sym_addr);
            }
        }
    }

    printf("LdCarley: Relocaciones completadas. Saltando a la aplicacion...\n");

    /* Llamar al punto de entrada original */
    void (*entry)(int, char **) = (void (*)(int, char **))app_entry;
    entry(argc, argv);

    /* Si el programa retorna, salimos */
    exit(0);
}
