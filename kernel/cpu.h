#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "gdt.h"

typedef struct {
    uint64_t cpu_id;
    void *current_task;
    gdt_entry_t gdt[7];
    gdt_ptr_t gdt_ptr;
    tss_t tss;
} cpu_local_t;

void cpu_enable_features(void);
void cpu_init_local(uint64_t id);
cpu_local_t *cpu_get_local(void);

#endif
