#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "boot_info.h"
#include "string.h"
#include "vmm.h"
#include "pmm.h"
#include "spinlock.h"

static uint64_t *kernel_pml4 = NULL;
static spinlock_t vmm_lock = 0;

static inline uint64_t get_hhdm_offset(void) {
    return HHDM_OFFSET;
}

static inline void *phys_to_virt(uintptr_t phys) {
    return (void *)(phys + get_hhdm_offset());
}

static inline uintptr_t virt_to_phys(void *virt) {
    return (uintptr_t)virt - get_hhdm_offset();
}

static uint64_t *get_next_table(uint64_t *table, uint64_t index, bool allocate) {
    if (table[index] & PTE_PRESENT) {
        return phys_to_virt(table[index] & ~0xFFFULL);
    }
    if (!allocate) return NULL;
    uintptr_t new_table_phys = (uintptr_t)pmm_alloc_page();
    if (!new_table_phys) return NULL;
    void *new_table_virt = phys_to_virt(new_table_phys);
    memset(new_table_virt, 0, PAGE_SIZE);
    table[index] = new_table_phys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    return new_table_virt;
}

uintptr_t virt_to_phys_in_pagemap(uint64_t *pml4, uintptr_t virt) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PTE_PRESENT)) return 0;
    uint64_t *pdpt = phys_to_virt(pml4[pml4_idx] & ~0xFFFULL);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return 0;
    uint64_t *pd = phys_to_virt(pdpt[pdpt_idx] & ~0xFFFULL);
    if (!(pd[pd_idx] & PTE_PRESENT)) return 0;
    uint64_t *pt = phys_to_virt(pd[pd_idx] & ~0xFFFULL);
    if (!(pt[pt_idx] & PTE_PRESENT)) return 0;

    return (pt[pt_idx] & ~0xFFFULL) + (virt & 0xFFF);
}

void vmm_init(boot_info_t *boot_info) {
    void *pml4_phys = pmm_alloc_page();
    kernel_pml4 = phys_to_virt((uintptr_t)pml4_phys);
    memset(kernel_pml4, 0, PAGE_SIZE);

    uint64_t offset = get_hhdm_offset();
    e820_entry_t *memmap = (e820_entry_t *)boot_info->memory_map_address;

    // 1. Mapear toda la memoria física en el HHDM
    for (uint32_t i = 0; i < boot_info->memory_map_count; i++) {
        uintptr_t base = (memmap[i].base / PAGE_SIZE) * PAGE_SIZE;
        uint64_t length = ((memmap[i].length + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
        for (uintptr_t j = 0; j < length; j += PAGE_SIZE) {
            vmm_map(kernel_pml4, base + j + offset, base + j, PTE_PRESENT | PTE_WRITABLE);
        }
    }

    // 2. Identity Mapping de los primeros 4GB (para kernel, stacks, boot info y framebuffer inicial)
    // Esto es necesario para que el código que aún usa direcciones físicas siga funcionando
    // y para que la transición a modo largo sea fluida.
    for (uintptr_t i = 0; i < 0x100000000ULL; i += PAGE_SIZE) {
        vmm_map(kernel_pml4, i, i, PTE_PRESENT | PTE_WRITABLE);
    }

    vmm_switch_pagemap(kernel_pml4);
}

uint64_t *vmm_create_pagemap(void) {
    void *pml4_phys = pmm_alloc_page();
    uint64_t *pml4 = phys_to_virt((uintptr_t)pml4_phys);
    memset(pml4, 0, PAGE_SIZE);

    // Copiar la mitad superior (Kernel & HHDM)
    for (int i = 256; i < 512; i++) pml4[i] = kernel_pml4[i];

    // También copiamos la entrada 0 para mantener el Identity Mapping en procesos de usuario
    // (Útil para acceder a estructuras de boot o framebuffer directamente si tienen permisos)
    pml4[0] = kernel_pml4[0];

    return pml4;
}

void vmm_map(uint64_t *pml4, uintptr_t virt, uintptr_t phys, uint64_t flags) {
    spin_lock(&vmm_lock);
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;
    uint64_t *pdpt = get_next_table(pml4, pml4_idx, true);
    uint64_t *pd = get_next_table(pdpt, pdpt_idx, true);
    uint64_t *pt = get_next_table(pd, pd_idx, true);
    pt[pt_idx] = phys | flags;
    spin_unlock(&vmm_lock);
}

void vmm_unmap(uint64_t *pml4, uintptr_t virt) {
    spin_lock(&vmm_lock);
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;
    uint64_t *pdpt = get_next_table(pml4, pml4_idx, false);
    if (!pdpt) { spin_unlock(&vmm_lock); return; }
    uint64_t *pd = get_next_table(pdpt, pdpt_idx, false);
    if (!pd) { spin_unlock(&vmm_lock); return; }
    uint64_t *pt = get_next_table(pd, pd_idx, false);
    if (!pt) { spin_unlock(&vmm_lock); return; }
    pt[pt_idx] = 0;
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
    spin_unlock(&vmm_lock);
}

void vmm_switch_pagemap(uint64_t *pml4) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(virt_to_phys(pml4)) : "memory");
}

uint64_t *vmm_get_kernel_pagemap(void) { return kernel_pml4; }
