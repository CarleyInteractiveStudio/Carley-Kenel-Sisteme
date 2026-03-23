#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "common/limine.h"
#include "common/string.h" // Cambiado para usar nuestra cabecera
#include "pmm.h"

/* Estructura para el mapa de memoria de Limine */
__attribute__((used, section(".requests")))
volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

/* Solicitud para el Higher Half Direct Map (HHDM) */
__attribute__((used, section(".requests")))
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

/* Variables globales para el estado del PMM */
static uint8_t *bitmap = NULL;
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t last_index = 0;
static uint64_t hhdm_offset = 0;

/* Funciones auxiliares para manipular el bitmap */
static inline void bitmap_set(uint64_t index) {
    bitmap[index / 8] |= (1 << (index % 8));
}

static inline void bitmap_clear(uint64_t index) {
    bitmap[index / 8] &= ~(1 << (index % 8));
}

static inline bool bitmap_test(uint64_t index) {
    return (bitmap[index / 8] & (1 << (index % 8))) != 0;
}

void pmm_init(void) {
    struct limine_memmap_response *memmap = memmap_request.response;
    if (memmap == NULL) return;

    if (hhdm_request.response) {
        hhdm_offset = hhdm_request.response->offset;
    }

    uint64_t highest_address = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE ||
            entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            uint64_t top = entry->base + entry->length;
            if (top > highest_address) highest_address = top;
        }
    }

    total_pages = highest_address / PAGE_SIZE;
    uint64_t bitmap_size = total_pages / 8;

    /* Buscar hueco para el bitmap */
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            bitmap = (uint8_t *)(entry->base + hhdm_offset);
            memset(bitmap, 0xff, bitmap_size);

            entry->base += bitmap_size;
            entry->length -= bitmap_size;
            break;
        }
    }

    /* Marcar como libres las regiones usables */
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            for (uint64_t j = 0; j < entry->length; j += PAGE_SIZE) {
                bitmap_clear((entry->base + j) / PAGE_SIZE);
                free_pages++;
            }
        }
    }
}

void *pmm_alloc_page(void) {
    for (uint64_t i = last_index; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            last_index = i;
            free_pages--;
            return (void *)(i * PAGE_SIZE);
        }
    }
    for (uint64_t i = 0; i < last_index; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            last_index = i;
            free_pages--;
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free_page(void *ptr) {
    uint64_t index = (uint64_t)ptr / PAGE_SIZE;
    if (index >= total_pages) return;
    if (bitmap_test(index)) {
        bitmap_clear(index);
        free_pages++;
        if (index < last_index) last_index = index;
    }
}

uint64_t pmm_get_total_memory(void) { return total_pages * PAGE_SIZE; }
uint64_t pmm_get_free_memory(void) { return free_pages * PAGE_SIZE; }
