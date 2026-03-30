#include "cpu.h"
#include "kheap.h"
#include "config.h"
#include <stddef.h>

#define MSR_GS_BASE 0xC0000101

void cpu_enable_features(void) {
    uint64_t cr0, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2);
    cr0 |= (1ULL << 1);
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 9);
    cr4 |= (1ULL << 10);
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    __asm__ volatile("finit");
}

void cpu_init_local(uint64_t id) {
    cpu_local_t *local = kmalloc(sizeof(cpu_local_t));
    // El scheduler/cpu_get_local espera direcciones virtuales Higher Half si HHDM esta activo.
    // Como usamos kmalloc que devuelve direcciones virtuales (que actualmente son identity),
    // pero el sistema podria cambiar, nos aseguramos que si HHDM_OFFSET != 0, estemos en el higher half.

    local->self = local;
    local->cpu_id = id;
    local->current_task = NULL;

    uint64_t addr = (uint64_t)local;
    uint32_t low = addr & 0xFFFFFFFF;
    uint32_t high = addr >> 32;
    __asm__ volatile("wrmsr" : : "c"(MSR_GS_BASE), "a"(low), "d"(high));
}

cpu_local_t *cpu_get_local(void) {
    cpu_local_t *local;
    __asm__ volatile("mov %%gs:0, %0" : "=r"(local));
    return local;
}
