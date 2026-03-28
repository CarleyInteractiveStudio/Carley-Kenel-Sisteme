#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Tamaño de página estándar x86_64: 4KB */
#define PAGE_SIZE 4096

/* Inicializa el Gestor de Memoria Física usando el mapa de memoria de Limine */
void pmm_init(void);

/* Reserva una página física de 4KB y devuelve su dirección física */
void *pmm_alloc_page(void);

/* Reserva 'count' páginas contiguas y devuelve su dirección física */
void *pmm_alloc_pages(size_t count);

/* Libera una página física en la dirección dada */
void pmm_free_page(void *ptr);

/* Libera 'count' páginas contiguas empezando por 'ptr' */
void pmm_free_pages(void *ptr, size_t count);

/* Devuelve la cantidad total de memoria RAM instalada (en bytes) */
uint64_t pmm_get_total_memory(void);

/* Devuelve la cantidad de memoria libre (en bytes) */
uint64_t pmm_get_free_memory(void);

#endif
