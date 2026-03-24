#ifndef GAME_INPUT_H
#define GAME_INPUT_H

#include <stdint.h>

typedef struct {
    int up, down, left, right;
    int btn_a, btn_b;
} controller_t;

/* Actualiza el estado del control basado en el teclado */
void game_input_update(controller_t *c);

#endif
