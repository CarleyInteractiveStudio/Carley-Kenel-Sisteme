#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

typedef struct {
    uint32_t status;      // 0: Descargando, 1: Cargando, 2: Lleno
    uint32_t percentage;
    uint32_t voltage;
} battery_info_t;

void battery_init(void);
battery_info_t battery_get_info(void);

#endif
