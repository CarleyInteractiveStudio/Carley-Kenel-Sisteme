#include "include/stdlib.h"
#include <stdint.h>

extern long syscall1(int num, long arg1);
#define SYS_SBRK 9

typedef struct block {
    size_t size;
    struct block *next;
    int free;
} block_t;

static block_t *free_list = NULL;

void *malloc(size_t size) {
    if (size == 0) return NULL;

    /* Alinear a 8 bytes */
    size = (size + 7) & ~7;

    /* Buscar en la lista de bloques libres (First Fit) */
    block_t *curr = free_list;
    while (curr) {
        if (curr->free && curr->size >= size) {
            curr->free = 0;
            return (void *)(curr + 1);
        }
        curr = curr->next;
    }

    /* Si no hay hueco, pedir más memoria al kernel */
    size_t total_size = size + sizeof(block_t);
    block_t *new_block = (block_t *)syscall1(SYS_SBRK, total_size);

    if ((long)new_block == -1) return NULL;

    new_block->size = size;
    new_block->free = 0;
    new_block->next = free_list;
    free_list = new_block;

    return (void *)(new_block + 1);
}

void free(void *ptr) {
    if (!ptr) return;
    block_t *block = (block_t *)ptr - 1;
    block->free = 1;
}

void exit(int status) {
    (void)status;
    /* Llamar a SYS_EXIT (8) */
    __asm__ volatile("mov $8, %%rax; int $0x80" : : : "rax");
}
