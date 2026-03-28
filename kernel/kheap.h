#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

/* Inicializa el montón del kernel */
void kheap_init(void);

/* Reserva 'size' bytes de memoria dinámica */
void *kmalloc(size_t size);

/* Libera memoria previamente reservada con kmalloc */
void kfree(void *ptr);

#endif
