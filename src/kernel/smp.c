#include "smp.h"
#include "drivers/acpi.h"
#include "drivers/apic.h"
#include "pmm.h"
#include "vmm.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "string.h"
#include "drivers/video.h"
#include "vfs.h"
#include "kheap.h"
#include "config.h"
#include "limine.h"

static int cpu_count = 1;
static volatile int cpus_started = 0;

void smp_init(void) {
    // Obsoleto.
}

static void ap_entry_limine(struct limine_smp_info *info) {
    (void)info;

    // Limine ya nos entra en 64-bit y con el stack configurado
    cpu_enable_features();
    gdt_init();
    idt_init();

    cpus_started++;

    // Bucle de espera para el scheduler
    for(;;) __asm__("hlt");
}

void smp_init_limine(struct limine_smp_response *response) {
    if (response == NULL) return;

    cpu_count = response->cpu_count;

    // El BSP es el CPU 0 (el que ya está corriendo kmain)
    for (uint64_t i = 0; i < response->cpu_count; i++) {
        struct limine_smp_info *cpu = response->cpus[i];

        if (cpu->lapic_id == response->bsp_lapic_id) {
            continue; // Saltar el BSP
        }

        // Arrancar el procesador secundario (AP)
        // Limine ejecutará ap_entry_limine en cada procesador
        cpu->goto_address = ap_entry_limine;
    }
}

void kmain_ap(void) {
    // Obsoleto.
}

int smp_get_cpu_count(void) { return cpu_count; }
