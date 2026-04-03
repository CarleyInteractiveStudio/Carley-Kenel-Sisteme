#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "boot_info.h"
#include "string.h"
#include "vmm.h"
#include "pmm.h"
#include "spinlock.h"
#include "config.h"

static uint64_t *kernel_pml4 = NULL;
static spinlock_t vmm_lock = 0;

static uint64_t current_hhdm = HHDM_OFFSET;

static inline void *phys_to_virt(uintptr_t phys) {
    return (void *)(phys + current_hhdm);
}

static inline uintptr_t virt_to_phys(void *virt) {
    return (uintptr_t)virt - current_hhdm;
}

static uint64_t *get_next_table(uint64_t *table, uint64_t index, bool allocate) {
    if (table[index] & PTE_PRESENT) {
        return phys_to_virt(table[index] & ~0xFFFULL);
    }
    if (!allocate) return NULL;
    void *new_table = pmm_alloc_page();
    if (!new_table) return NULL;
    memset(phys_to_virt((uintptr_t)new_table), 0, PAGE_SIZE);
    table[index] = (uintptr_t)new_table | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    return phys_to_virt((uintptr_t)new_table);
}

void vmm_init(boot_info_t *boot_info) {
    vmm_init_limine(boot_info, 0x100000, 0xFFFF800000100000);
}

void vmm_init_limine(boot_info_t *boot_info, uint64_t kernel_phys, uint64_t kernel_virt) {
    void *pml4_phys = pmm_alloc_page();
    kernel_pml4 = phys_to_virt((uintptr_t)pml4_phys);
    memset(kernel_pml4, 0, PAGE_SIZE);

    e820_entry_t *map = (e820_entry_t *)boot_info->memory_map_address;

    // Mapear memoria física en HHDM
    for (uint32_t i = 0; i < boot_info->memory_map_count; i++) {
        uintptr_t base = (map[i].base / PAGE_SIZE) * PAGE_SIZE;
        uint64_t length = ((map[i].length + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
        for (uintptr_t j = 0; j < length; j += PAGE_SIZE) {
            vmm_map(kernel_pml4, base + j + current_hhdm, base + j, PTE_PRESENT | PTE_WRITABLE);
            vmm_map(kernel_pml4, base + j, base + j, PTE_PRESENT | PTE_WRITABLE);
        }
    }

    // Mapear Kernel
    for (uintptr_t i = 0; i < 0x2000000; i += PAGE_SIZE) {
        vmm_map(kernel_pml4, kernel_virt + i, kernel_phys + i, PTE_PRESENT | PTE_WRITABLE);
    }

    // Mapear Framebuffer
    uintptr_t fb_base = boot_info->framebuffer_address;
    uint64_t fb_size = boot_info->screen_width * boot_info->screen_height * 4;
    for (uintptr_t i = 0; i < fb_size; i += PAGE_SIZE) {
        vmm_map(kernel_pml4, fb_base + i + current_hhdm, fb_base + i, PTE_PRESENT | PTE_WRITABLE);
        vmm_map(kernel_pml4, fb_base + i, fb_base + i, PTE_PRESENT | PTE_WRITABLE);
    }

    vmm_switch_pagemap(kernel_pml4);
}

uint64_t *vmm_create_pagemap(void) {
    void *pml4_phys = pmm_alloc_page();
    uint64_t *pml4 = phys_to_virt((uintptr_t)pml4_phys);
    memset(pml4, 0, PAGE_SIZE);
    for (int i = 256; i < 512; i++) pml4[i] = kernel_pml4[i];
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

void vmm_switch_pagemap(uint64_t *pml4) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(virt_to_phys(pml4)) : "memory");
}

uint64_t *vmm_get_kernel_pagemap(void) { return kernel_pml4; }

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
