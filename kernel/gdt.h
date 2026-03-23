#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Estructura para una entrada de la GDT (64 bits) */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

/* Estructura para el puntero de la GDT (GDTR) */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

/* Inicializa la Tabla de Descriptores Globales */
void gdt_init(void);

#endif
