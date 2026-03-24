#include "vfs.h"
#include "vmm.h"
#include "pmm.h"
#include "kheap.h"
#include "elf.h"
#include "sched.h"
#include "common/string.h"
#include "common/limine.h"

extern volatile struct limine_hhdm_request hhdm_request;

/* Carga un archivo ELF y prepara el stack con argumentos */
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
    uint64_t hhdm = hhdm_request.response->offset;

    Elf64_Phdr *phdrs = (Elf64_Phdr *)(buffer + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            size_t pages = (phdrs[i].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
            for (size_t j = 0; j < pages; j++) {
                void *phys = pmm_alloc_page();
                vmm_map(pagemap, phdrs[i].p_vaddr + (j * PAGE_SIZE), (uintptr_t)phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
                size_t to_copy = (phdrs[i].p_filesz > j * PAGE_SIZE) ? phdrs[i].p_filesz - j * PAGE_SIZE : 0;
                if (to_copy > PAGE_SIZE) to_copy = PAGE_SIZE;
                if (to_copy > 0) memcpy((void *)((uintptr_t)phys + hhdm), buffer + phdrs[i].p_offset + (j * PAGE_SIZE), to_copy);
                else memset((void *)((uintptr_t)phys + hhdm), 0, PAGE_SIZE);
            }
        }
    }

    /* Preparar ARGC/ARGV en el stack de usuario */
    /* El kernel debe mapear y escribir en el stack del usuario via HHDM */
    uintptr_t stack_top_hhdm = (uintptr_t)virt_to_phys_in_pagemap(new_task->pml4, 0x70000000000 + (2 * PAGE_SIZE) - 8) + hhdm;
    // (Implementación simplificada para la demo: por ahora solo pasamos argc en RDI y argv en RSI)
    new_task->context->rdi = argc;
    new_task->context->rsi = (uint64_t)argv; // NOTA: argv debe ser una direccion de usuario valida.

    new_task->context->rip = ehdr->e_entry;
    kfree(buffer);
    return 0;
}

int elf_load(const char *path) {
    return elf_load_ext(path, 0, NULL);
}
