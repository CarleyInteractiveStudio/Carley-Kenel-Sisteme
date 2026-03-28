#include "include/time.h"
#include <stdint.h>

extern long syscall1(int num, long arg1);
#define SYS_TIME 12

time_t time(time_t *tloc) {
    /* Por simplicidad, esta LibC devuelve una estructura de tiempo
       o ticks si tloc es NULL */
    if (tloc) {
        syscall1(SYS_TIME, (long)tloc);
    }
    return 0;
}
