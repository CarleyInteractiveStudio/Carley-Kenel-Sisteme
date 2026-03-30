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

static int cpu_count = 0;
static volatile int cpus_started = 0;

void smp_init(void) {
    acpi_header_t *madt = (acpi_header_t *)acpi_find_table("APIC");
    if (!madt) return;

    uint8_t *ptr = (uint8_t *)madt + sizeof(acpi_header_t) + 8;
    uint8_t *end = (uint8_t *)madt + madt->length;

    // 1. Cargar el Trampoline en 0x1000 (Dirección física baja)
    // Buscamos el archivo en el VFS (montado desde el initrd o carleyfs)
    vfs_node_t *tramp_node = vfs_open("ap_trampoline");
    if (tramp_node) {
        uint8_t *tramp_buf = (uint8_t *)kmalloc(tramp_node->size);
        vfs_read(tramp_node, 0, tramp_node->size, tramp_buf);
        // Copiar a la dirección física 0x1000 (accesible via identity o HHDM)
        memcpy((void *)(0x1000 + HHDM_OFFSET), tramp_buf, tramp_node->size);
        kfree(tramp_buf);
    }

    // 2. Iterar sobre los Local APICs en el MADT
    while (ptr < end) {
        uint8_t type = ptr[0];
        uint8_t len = ptr[1];

        if (type == 0) { // Local APIC
            uint8_t apic_id = ptr[3];
            uint8_t flags = ptr[4];

            if ((flags & 1) && apic_id != lapic_get_id()) {
                // Arrancar CPU
                cpu_count++;

                // Preparar stack y entry point para el AP en 0x1500 (físico)
                void *stack = pmm_alloc_page();
                *(uint64_t *)(0x1500 + HHDM_OFFSET) = (uint64_t)stack + 4096 + HHDM_OFFSET;
                *(uint64_t *)(0x1508 + HHDM_OFFSET) = (uint64_t)kmain_ap;

                // INIT IPI
                apic_send_ipi(apic_id, 0x00000500);
                // Delay
                for(volatile int i=0; i<1000000; i++);
                // STARTUP IPI (Vector 0x01 -> 0x01 * 0x1000 = 0x1000)
                apic_send_ipi(apic_id, 0x00000601);
            }
        }
        ptr += len;
    }

    cpu_count++; // Contar el BSP
}

void kmain_ap(void) {
    cpu_enable_features();
    // vmm_switch_pagemap se heredó vía CR3 en el trampoline
    gdt_init();
    idt_init();

    cpus_started++;

    // El AP entra en el scheduler
    while(1) __asm__("hlt");
}

int smp_get_cpu_count(void) { return cpu_count; }
