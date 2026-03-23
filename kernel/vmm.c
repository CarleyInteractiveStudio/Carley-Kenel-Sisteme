#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "common/limine.h"
#include "common/string.h"
#include "vmm.h"
#include "pmm.h"

/* Solicitud para el kernel address */
__attribute__((used, section(".requests")))
volatile struct limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

/* Solicitud para el HHDM */
extern volatile struct limine_hhdm_request hhdm_request;

static uint64_t *kernel_pml4 = NULL;

/* Funciones auxiliares para obtener el offset HHDM */
static inline uint64_t get_hhdm_offset(void) {
    return hhdm_request.response->offset;
}

/* Convierte una dirección física a virtual usando HHDM */
static inline void *phys_to_virt(uintptr_t phys) {
    return (void *)(phys + get_hhdm_offset());
}

/* Convierte una dirección virtual HHDM a física */
static inline uintptr_t virt_to_phys(void *virt) {
    return (uintptr_t)virt - get_hhdm_offset();
}

/* Obtiene la siguiente tabla de páginas de un nivel inferior */
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

void vmm_init(void) {
    /* Crear el PML4 del kernel */
    void *pml4_phys = pmm_alloc_page();
    kernel_pml4 = phys_to_virt((uintptr_t)pml4_phys);
    memset(kernel_pml4, 0, PAGE_SIZE);

    /* Mapear los primeros 4GB de memoria física (HHDM) */
    uint64_t offset = get_hhdm_offset();
    for (uintptr_t i = 0; i < 0x100000000; i += PAGE_SIZE) {
        vmm_map(kernel_pml4, i + offset, i, PTE_PRESENT | PTE_WRITABLE);
    }

    /* Mapear el kernel (basado en la respuesta de Limine) */
    struct limine_kernel_address_response *ka = kernel_address_request.response;
    for (uintptr_t i = 0; i < 0x2000000; i += PAGE_SIZE) { // Mapeamos 32MB para estar seguros
        vmm_map(kernel_pml4, ka->virtual_base + i, ka->physical_base + i, PTE_PRESENT | PTE_WRITABLE);
    }

    /* Activar el nuevo mapa de páginas */
    vmm_switch_pagemap(kernel_pml4);
}

void vmm_map(uint64_t *pml4, uintptr_t virt, uintptr_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_next_table(pml4, pml4_idx, true);
    uint64_t *pd = get_next_table(pdpt, pdpt_idx, true);
    uint64_t *pt = get_next_table(pd, pd_idx, true);

    pt[pt_idx] = phys | flags;
}

void vmm_unmap(uint64_t *pml4, uintptr_t virt) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_next_table(pml4, pml4_idx, false);
    if (!pdpt) return;
    uint64_t *pd = get_next_table(pdpt, pdpt_idx, false);
    if (!pd) return;
    uint64_t *pt = get_next_table(pd, pd_idx, false);
    if (!pt) return;

    pt[pt_idx] = 0;
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_switch_pagemap(uint64_t *pml4) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(virt_to_phys(pml4)) : "memory");
}

uint64_t *vmm_get_kernel_pagemap(void) {
    return kernel_pml4;
}
