#include "include/stdlib.h"
#include "include/string.h"
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
    size = (size + 7) & ~7;
    block_t *curr = free_list;
    while (curr) {
        if (curr->free && curr->size >= size) {
            curr->free = 0;
            return (void *)(curr + 1);
        }
        curr = curr->next;
    }
    size_t total_size = size + sizeof(block_t);
    block_t *new_block = (block_t *)syscall1(SYS_SBRK, total_size);
    if ((long)new_block == -1) return NULL;
    new_block->size = size;
    new_block->free = 0;
    new_block->next = free_list;
    free_list = new_block;
    return (void *)(new_block + 1);
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }

    block_t *block = (block_t *)ptr - 1;
    if (block->size >= size) return ptr;

    void *new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        free(ptr);
    }
    return new_ptr;
}

void free(void *ptr) {
    if (!ptr) return;
    block_t *block = (block_t *)ptr - 1;
    block->free = 1;
}

void exit(int status) {
    (void)status;
    __asm__ volatile("mov $8, %%rax; int $0x80" : : : "rax");
}

int atoi(const char *nptr) {
    int res = 0;
    while (*nptr >= '0' && *nptr <= '9') {
        res = res * 10 + (*nptr - '0');
        nptr++;
    }
    return res;
}

extern long syscall2(int num, long arg1, long arg2);
#define SYS_CREATE 17
#define SYS_MKDIR 18

int mkdir(const char *path) {
    return (int)syscall1(SYS_MKDIR, (long)path);
}

int mkfile(const char *path, uint32_t size) {
    return (int)syscall2(SYS_CREATE, (long)path, (long)size);
}
