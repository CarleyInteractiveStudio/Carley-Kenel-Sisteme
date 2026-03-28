#ifndef LIB_TIME_H
#define LIB_TIME_H

#include <stdint.h>

typedef long time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
};

/* Devuelve los ticks del sistema (vía syscall yield que devuelve tiempo en RAX) */
/* O una syscall dedicada */
time_t time(time_t *tloc);

#endif
