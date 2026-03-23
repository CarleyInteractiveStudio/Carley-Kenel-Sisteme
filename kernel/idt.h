#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include "sched.h"

/* Estructura para una entrada de la IDT (64 bits) */
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

/* Estructura para el puntero de la IDT (IDTR) */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

/* Inicializa la Tabla de Descriptores de Interrupciones */
void idt_init(void);

/* Configura una entrada específica de la IDT */
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);

#endif
