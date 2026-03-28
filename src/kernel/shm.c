#include "shm.h"
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "spinlock.h"
#include "string.h"

static shm_segment_t shm_segments[MAX_SHM_SEGMENTS];
static spinlock_t shm_lock = 0;

void shm_init(void) {
    memset(shm_segments, 0, sizeof(shm_segments));
}

long shm_get(uint64_t id, size_t size) {
    spin_lock(&shm_lock);

    // Buscar si ya existe
    for (int i = 0; i < MAX_SHM_SEGMENTS; i++) {
        if (shm_segments[i].used && shm_segments[i].id == id) {
            spin_unlock(&shm_lock);
            return i;
        }
    }

    // Crear uno nuevo
    for (int i = 0; i < MAX_SHM_SEGMENTS; i++) {
        if (!shm_segments[i].used) {
            size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
            void *phys = pmm_alloc_pages(pages);
            if (!phys) { spin_unlock(&shm_lock); return -1; }

            shm_segments[i].id = id;
            shm_segments[i].phys_addr = phys;
            shm_segments[i].size = pages * PAGE_SIZE;
            shm_segments[i].used = true;

            spin_unlock(&shm_lock);
            return i;
        }
    }

    spin_unlock(&shm_lock);
    return -1;
}

void *shm_at(uint64_t shm_id, uintptr_t hint) {
    if (shm_id >= MAX_SHM_SEGMENTS || !shm_segments[shm_id].used) return NULL;

    task_t *curr = sched_get_current_task();
    uintptr_t virt = hint;
    if (virt == 0) {
        // Encontrar hueco en el heap del usuario
        virt = curr->heap_end;
        curr->heap_end += shm_segments[shm_id].size;
    }

    size_t pages = shm_segments[shm_id].size / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        vmm_map(curr->pml4, virt + (i * PAGE_SIZE), (uintptr_t)shm_segments[shm_id].phys_addr + (i * PAGE_SIZE), PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    }

    return (void *)virt;
}
