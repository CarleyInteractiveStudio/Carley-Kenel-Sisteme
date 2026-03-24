#include "cpu.h"

void cpu_enable_features(void) {
    uint64_t cr0, cr4;

    /* Habilitar FPU y SSE */
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2); // Limpiar EM (Emulation)
    cr0 |= (1ULL << 1);  // Set MP (Monitor Coprocessor)
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 9);  // Set OSFXSR (FXSAVE/FXRSTOR support)
    cr4 |= (1ULL << 10); // Set OSXMMEXCPT (SIMD Exception support)
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));

    /* Inicializar FPU */
    __asm__ volatile("finit");
}
