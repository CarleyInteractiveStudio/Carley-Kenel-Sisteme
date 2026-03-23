#include "vfs.h"
#include "vmm.h"
#include "pmm.h"
#include "kheap.h"
#include "elf.h"
#include "sched.h"
#include "common/string.h"

int elf_load(const char *path) {
    /* Por simplicidad en la demo, como solo hay un archivo, usamos vfs_root directamente */
    vfs_node_t *node = vfs_root;
    if (!node) return -1;

    uint8_t *buffer = kmalloc(node->size);
    vfs_read(node, 0, node->size, buffer);

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)buffer;
    if (memcmp(ehdr->e_ident, "\x7f\x45\x4c\x46", 4) != 0) {
        kfree(buffer);
        return -1;
    }

    task_t *new_task = sched_create_task(NULL, true);
    uint64_t *pagemap = new_task->pml4;

    Elf64_Phdr *phdrs = (Elf64_Phdr *)(buffer + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            size_t pages = (phdrs[i].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
            for (size_t j = 0; j < pages; j++) {
                void *phys = pmm_alloc_page();
                vmm_map(pagemap, phdrs[i].p_vaddr + (j * PAGE_SIZE), (uintptr_t)phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER);

                /* Copia simplificada: usamos el hecho de que el kernel tiene HHDM para escribir directo */
                /* En un sistema real usaríamos un mapeo temporal o copiaríamos antes de cambiar CR3 */
                extern volatile struct limine_hhdm_request hhdm_request;
                uint64_t hhdm = hhdm_request.response->offset;

                size_t to_copy = (phdrs[i].p_filesz > j * PAGE_SIZE) ? phdrs[i].p_filesz - j * PAGE_SIZE : 0;
                if (to_copy > PAGE_SIZE) to_copy = PAGE_SIZE;

                if (to_copy > 0) {
                    memcpy((void *)((uintptr_t)phys + hhdm), buffer + phdrs[i].p_offset + (j * PAGE_SIZE), to_copy);
                }
            }
        }
    }

    new_task->context->rip = ehdr->e_entry;

    kfree(buffer);
    return 0;
}
