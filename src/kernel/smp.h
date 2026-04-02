#ifndef SMP_H
#define SMP_H

#include <stdint.h>

struct limine_smp_response;

void smp_init(void);
void smp_init_limine(struct limine_smp_response *response);

void kmain_ap(void);
int smp_get_cpu_count(void);

#endif
