#include "pit.h"
#include "io.h"
#include "idt.h"

static uint64_t ticks = 0;

/* Manejador de la interrupción del temporizador (IRQ0) */
void timer_handler(void) {
    ticks++;
    /* EOI se maneja en idt.c para evitar duplicaciones */
}

void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;

    /* Enviar el comando al PIT */
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

uint64_t pit_get_ticks(void) {
    return ticks;
}
