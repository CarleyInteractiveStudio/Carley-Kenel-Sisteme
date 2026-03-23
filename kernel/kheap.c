#include <stdint.h>
#include <stddef.h>
#include "kheap.h"
#include "pmm.h"
#include "vmm.h"

/* Estructura simple para un nodo del montón (heap) */
typedef struct heap_node {
    size_t size;
    struct heap_node *next;
    bool free;
} heap_node_t;

#define HEAP_START 0xFFFF900000000000
#define INITIAL_PAGES 16

static heap_node_t *head = NULL;

void kheap_init(void) {
    /* Reservar unas cuantas páginas iniciales para el montón */
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
    heap_node_t *curr = head;
    while (curr) {
        if (curr->free && curr->size >= size) {
            /* Si hay suficiente espacio extra, dividir el nodo */
            if (curr->size > size + sizeof(heap_node_t) + 8) {
                heap_node_t *new_node = (heap_node_t *)((uint8_t *)curr + sizeof(heap_node_t) + size);
                new_node->size = curr->size - size - sizeof(heap_node_t);
                new_node->free = true;
                new_node->next = curr->next;

                curr->size = size;
                curr->next = new_node;
            }
            curr->free = false;
            return (void *)((uint8_t *)curr + sizeof(heap_node_t));
        }
        curr = curr->next;
    }
    return NULL; // El montón se ha agotado
}

void kfree(void *ptr) {
    if (!ptr) return;
    heap_node_t *node = (heap_node_t *)((uint8_t *)ptr - sizeof(heap_node_t));
    node->free = true;

    /* Fusión básica con el siguiente nodo si también está libre */
    if (node->next && node->next->free) {
        node->size += sizeof(heap_node_t) + node->next->size;
        node->next = node->next->next;
    }
}
