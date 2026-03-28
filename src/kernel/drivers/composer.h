#ifndef COMPOSER_H
#define COMPOSER_H

#include <stdint.h>
#include <stdbool.h>
#include "ipc.h"

#define COMPOSER_DRAW_PIXEL 1
#define COMPOSER_DRAW_RECT  2
#define COMPOSER_DRAW_CHAR  3
#define COMPOSER_CLEAR      4
#define COMPOSER_DRAW_SPRITE 5
#define COMPOSER_LOAD_ICON   6
#define COMPOSER_CREATE_WINDOW 10
#define COMPOSER_MOVE_WINDOW   11
#define COMPOSER_ATTACH_SHM    12

typedef struct {
    uint32_t x, y, w, h;
    uint32_t target_w, target_h; // For animations
    uint32_t owner_id;
    uint32_t *shm_buffer;
    bool active;
    bool animating;
} wm_window_t;

#define MAX_WINDOWS 16

/* Inicia el hilo del servidor gráfico */
void composer_start(void);

#endif
