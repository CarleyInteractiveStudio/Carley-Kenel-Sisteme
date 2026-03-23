#ifndef VMM_H
#define VMM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Flags para las entradas de la tabla de páginas */
#define PTE_PRESENT (1ULL << 0)
#define PTE_WRITABLE (1ULL << 1)
#define PTE_USER (1ULL << 2)

/* Inicializa el Gestor de Memoria Virtual */
void vmm_init(void);

/* Mapea una dirección virtual a una física */
void vmm_map(uint64_t *pml4, uintptr_t virt, uintptr_t phys, uint64_t flags);

/* Desmapea una dirección virtual */
void vmm_unmap(uint64_t *pml4, uintptr_t virt);

/* Cambia el espacio de direcciones actual (Carga un nuevo PML4) */
void vmm_switch_pagemap(uint64_t *pml4);

/* Obtiene el PML4 actual del kernel */
uint64_t *vmm_get_kernel_pagemap(void);

#endif
