#ifndef PIT_H
#define PIT_H

#include <stdint.h>

/* Inicializa el PIT para disparar interrupciones a una frecuencia dada (Hz) */
void pit_init(uint32_t frequency);

/* Devuelve el número de ticks desde el arranque */
uint64_t pit_get_ticks(void);

#endif
