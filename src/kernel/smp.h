#ifndef SMP_H
#define SMP_H

#include <stdint.h>

void smp_init(void);
void kmain_ap(void);
int smp_get_cpu_count(void);

#endif
