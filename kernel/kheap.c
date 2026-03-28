#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kheap.h"
#include "pmm.h"
#include "vmm.h"
#include "spinlock.h"

typedef struct heap_node {
    size_t size;
    struct heap_node *next;
    bool free;
} heap_node_t;

#define HEAP_START 0xFFFF900000000000
#define INITIAL_PAGES 16

static heap_node_t *head = NULL;
static spinlock_t heap_lock = 0;

void kheap_init(void) {
    for (size_t i = 0; i < INITIAL_PAGES; i++) {
        void *phys = pmm_alloc_page();
        vmm_map(vmm_get_kernel_pagemap(), HEAP_START + (i * PAGE_SIZE), (uintptr_t)phys, PTE_PRESENT | PTE_WRITABLE);
    }

    head = (heap_node_t *)HEAP_START;
    head->size = (INITIAL_PAGES * PAGE_SIZE) - sizeof(heap_node_t);
    head->next = NULL;
    head->free = true;
}

void *kmalloc(size_t size) {
    spin_lock(&heap_lock);
    heap_node_t *curr = head;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (curr->size > size + sizeof(heap_node_t) + 8) {
                heap_node_t *new_node = (heap_node_t *)((uint8_t *)curr + sizeof(heap_node_t) + size);
                new_node->size = curr->size - size - sizeof(heap_node_t);
                new_node->free = true;
                new_node->next = curr->next;

                curr->size = size;
                curr->next = new_node;
            }
            curr->free = false;
            spin_unlock(&heap_lock);
            return (void *)((uint8_t *)curr + sizeof(heap_node_t));
        }
        curr = curr->next;
    }
    spin_unlock(&heap_lock);
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    spin_lock(&heap_lock);
    heap_node_t *node = (heap_node_t *)((uint8_t *)ptr - sizeof(heap_node_t));
    node->free = true;

    if (node->next && node->next->free) {
        node->size += sizeof(heap_node_t) + node->next->size;
        node->next = node->next->next;
    }
    spin_unlock(&heap_lock);
}
