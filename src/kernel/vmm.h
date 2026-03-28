#ifndef VMM_H
#define VMM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "boot_info.h"

#define PTE_PRESENT (1ULL << 0)
#define PTE_WRITABLE (1ULL << 1)
#define PTE_USER (1ULL << 2)

void vmm_init(boot_info_t *boot_info);
void vmm_map(uint64_t *pml4, uintptr_t virt, uintptr_t phys, uint64_t flags);
void vmm_unmap(uint64_t *pml4, uintptr_t virt);
void vmm_switch_pagemap(uint64_t *pml4);
uint64_t *vmm_get_kernel_pagemap(void);
uint64_t *vmm_create_pagemap(void);

/* Nueva: convierte direccion virtual de un pagemap especifico a fisica */
uintptr_t virt_to_phys_in_pagemap(uint64_t *pml4, uintptr_t virt);

#endif
