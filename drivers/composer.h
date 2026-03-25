#ifndef COMPOSER_H
#define COMPOSER_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/ipc.h"

#define COMPOSER_DRAW_PIXEL 1
#define COMPOSER_DRAW_RECT  2
#define COMPOSER_DRAW_CHAR  3
#define COMPOSER_CLEAR      4
#define COMPOSER_DRAW_SPRITE 5
#define COMPOSER_CREATE_WINDOW 10
#define COMPOSER_MOVE_WINDOW   11

typedef struct {
    uint32_t x, y, w, h;
    uint32_t owner_id;
    bool active;
} wm_window_t;

#define MAX_WINDOWS 16

/* Inicia el hilo del servidor gráfico */
void composer_start(void);

#endif
