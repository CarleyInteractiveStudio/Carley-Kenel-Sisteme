#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "common/limine.h"
#include "common/string.h"
#include "pmm.h"
#include "spinlock.h"

extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;

static uint8_t *bitmap = NULL;
static uint64_t total_pages = 0;
static uint64_t free_pages = 0;
static uint64_t last_index = 0;
static uint64_t hhdm_offset = 0;
static spinlock_t pmm_lock = 0;

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

    if (hhdm_request.response) hhdm_offset = hhdm_request.response->offset;

    uint64_t highest_address = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            uint64_t top = entry->base + entry->length;
            if (top > highest_address) highest_address = top;
        }
    }

    total_pages = highest_address / PAGE_SIZE;
    uint64_t bitmap_size = (total_pages / 8);
    bitmap_size = (bitmap_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    /* Buscar hueco para el bitmap */
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            bitmap = (uint8_t *)(entry->base + hhdm_offset);
            memset(bitmap, 0xff, bitmap_size);

            /* Marcar el bitmap como ocupado inmediatamente */
            uint64_t start_page = entry->base / PAGE_SIZE;
            uint64_t pages_needed = bitmap_size / PAGE_SIZE;

            /* No usamos bitmap_set aquí porque aún no hemos terminado pmm_init */
            /* En lugar de eso, reducimos el segmento usable de la entrada */
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
    spin_lock(&pmm_lock);
    for (uint64_t i = last_index; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            last_index = i;
            free_pages--;
            spin_unlock(&pmm_lock);
            return (void *)(i * PAGE_SIZE);
        }
    }
    for (uint64_t i = 0; i < last_index; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            last_index = i;
            free_pages--;
            spin_unlock(&pmm_lock);
            return (void *)(i * PAGE_SIZE);
        }
    }
    spin_unlock(&pmm_lock);
    return NULL;
}

void *pmm_alloc_pages(size_t count) {
    spin_lock(&pmm_lock);
    size_t consecutive = 0;
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            consecutive++;
            if (consecutive == count) {
                uint64_t start = i - count + 1;
                for (uint64_t j = start; j <= i; j++) bitmap_set(j);
                free_pages -= count;
                spin_unlock(&pmm_lock);
                return (void *)(start * PAGE_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }
    spin_unlock(&pmm_lock);
    return NULL;
}

void pmm_free_page(void *ptr) {
    uint64_t index = (uint64_t)ptr / PAGE_SIZE;
    if (index >= total_pages) return;
    spin_lock(&pmm_lock);
    if (bitmap_test(index)) {
        bitmap_clear(index);
        free_pages++;
        if (index < last_index) last_index = index;
    }
    spin_unlock(&pmm_lock);
}

void pmm_free_pages(void *ptr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        pmm_free_page((void *)((uintptr_t)ptr + (i * PAGE_SIZE)));
    }
}

uint64_t pmm_get_total_memory(void) { return total_pages * PAGE_SIZE; }
uint64_t pmm_get_free_memory(void) { return free_pages * PAGE_SIZE; }
