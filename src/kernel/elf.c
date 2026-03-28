#include "vfs.h"
#include "vmm.h"
#include "pmm.h"
#include "kheap.h"
#include "elf.h"
#include "sched.h"
#include "string.h"
#include "limine.h"

extern volatile struct limine_hhdm_request hhdm_request;

static int elf_load_into_pagemap(uint64_t *pagemap, uint8_t *buffer) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)buffer;
    Elf64_Phdr *phdrs = (Elf64_Phdr *)(buffer + ehdr->e_phoff);
    uint64_t hhdm = hhdm_request.response->offset;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            size_t pages = (phdrs[i].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
            for (size_t j = 0; j < pages; j++) {
                uintptr_t vaddr = phdrs[i].p_vaddr + (j * PAGE_SIZE);
                // Si la página ya está mapeada (por ejemplo, por el intérprete), no la sobreescribimos
                if (virt_to_phys_in_pagemap(pagemap, vaddr) == 0) {
                    void *phys = pmm_alloc_page();
                    vmm_map(pagemap, vaddr, (uintptr_t)phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
                    size_t to_copy = (phdrs[i].p_filesz > j * PAGE_SIZE) ? phdrs[i].p_filesz - j * PAGE_SIZE : 0;
                    if (to_copy > PAGE_SIZE) to_copy = PAGE_SIZE;
                    if (to_copy > 0) memcpy((void *)((uintptr_t)phys + hhdm), buffer + phdrs[i].p_offset + (j * PAGE_SIZE), to_copy);
                    else memset((void *)((uintptr_t)phys + hhdm), 0, PAGE_SIZE);
                }
            }
        }
    }
    return 0;
}

int elf_load_ext(const char *path, int argc, char **argv) {
    vfs_node_t *node = vfs_open(path);
    if (!node) return -1;

    uint8_t *buffer = kmalloc(node->size);
    if (!buffer) return -1;
    vfs_read(node, 0, node->size, buffer);

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)buffer;
    if (memcmp(ehdr->e_ident, "\x7f\x45\x4c\x46", 4) != 0) {
        kfree(buffer); return -1;
    }

    task_t *new_task = sched_create_task(NULL, true);
    uint64_t *pagemap = new_task->pml4;

    /* Asignar capacidades por defecto basadas en el nombre */
    if (strcmp(path, "shell.elf") == 0) new_task->capabilities = 0xFFFFFFFF; // Shell tiene todo
    else if (strcmp(path, "input.elf") == 0) new_task->capabilities = CAP_HARDWARE;
    else if (strcmp(path, "dashboard.elf") == 0) new_task->capabilities = CAP_DISK;
    else new_task->capabilities = CAP_NONE;

    uint64_t entry_point = ehdr->e_entry;
    char *interp_path = NULL;

    Elf64_Phdr *phdrs = (Elf64_Phdr *)(buffer + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_INTERP) {
            interp_path = (char *)(buffer + phdrs[i].p_offset);
            break;
        }
    }

    elf_load_into_pagemap(pagemap, buffer);

    if (interp_path) {
        /* Cargar el enlazador dinamico */
        vfs_node_t *interp_node = vfs_open(interp_path);
        if (interp_node) {
            uint8_t *interp_buffer = kmalloc(interp_node->size);
            vfs_read(interp_node, 0, interp_node->size, interp_buffer);
            Elf64_Ehdr *interp_ehdr = (Elf64_Ehdr *)interp_buffer;
            elf_load_into_pagemap(pagemap, interp_buffer);
            entry_point = interp_ehdr->e_entry;
            kfree(interp_buffer);
        }

        /* En el futuro, el propio ldso abrirá y mapeará libc.so usando syscalls (mmap, open, read) */
    }

    // Pasamos argc y argv al nuevo proceso
    new_task->context->rdi = argc;
    new_task->context->rsi = (uint64_t)argv;
    new_task->context->rip = entry_point;
    // RDX suele pasar la direccion de terminacion o info del loader en algunos ABIs
    new_task->context->rdx = ehdr->e_entry;

    kfree(buffer);
    return 0;
}

int elf_load(const char *path) {
    return elf_load_ext(path, 0, NULL);
}
