#ifndef COMPOSER_H
#define COMPOSER_H

#include <stdint.h>
#include "kernel/ipc.h"

#define COMPOSER_DRAW_PIXEL 1
#define COMPOSER_DRAW_RECT  2
#define COMPOSER_DRAW_CHAR  3
#define COMPOSER_CLEAR      4
#define COMPOSER_DRAW_SPRITE 5

/* Inicia el hilo del servidor gráfico */
void composer_start(void);

#endif
