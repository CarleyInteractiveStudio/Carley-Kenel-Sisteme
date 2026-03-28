#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine.h"
#include "boot_info.h"
#include "string.h"
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
    // Obsoleto, redirigimos a la version custom si Limine no esta presente
    // En un sistema real, Limine rellenaria las estructuras, pero aqui usamos el cargador Carley
}

void pmm_init_custom(uint64_t map_addr, uint32_t count) {
    e820_entry_t *map = (e820_entry_t *)map_addr;
    hhdm_offset = 0; // En nuestro cargador el kernel es identity mapped (0-4GB)

    uint64_t highest_address = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (map[i].type == 1) { // Type 1 = Usable
            uint64_t top = map[i].base + map[i].length;
            if (top > highest_address) highest_address = top;
        }
    }

    total_pages = highest_address / PAGE_SIZE;
    uint64_t bitmap_size = (total_pages / 8);
    bitmap_size = (bitmap_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    // Buscar sitio para el bitmap. Usaremos los primeros 1MB libres despues del Kernel
    // El kernel esta en 0x100000. Pondremos el bitmap en 0x500000 (5MB) para estar seguros.
    bitmap = (uint8_t *)0x500000;
    memset(bitmap, 0xff, bitmap_size);

    for (uint32_t i = 0; i < count; i++) {
        if (map[i].type == 1) {
            for (uint64_t j = 0; j < map[i].length; j += PAGE_SIZE) {
                uint64_t addr = map[i].base + j;
                // No marcar como libre si esta por debajo de 6MB (Kernel, Bootloader, Stack, Bitmap)
                if (addr >= 0x600000) {
                    bitmap_clear(addr / PAGE_SIZE);
                    free_pages++;
                }
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
