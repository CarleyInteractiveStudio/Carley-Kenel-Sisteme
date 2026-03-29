#include "apic.h"
#include "acpi.h"
#include "io.h"
#include "string.h"
#include "pmm.h"
#include "config.h"

static uintptr_t lapic_base = 0;

void apic_init(void) {
    acpi_header_t *madt = acpi_find_table("APIC");
    if (!madt) return;

    // MADT: Local APIC Address at offset 36
    lapic_base = *(uint32_t*)((uintptr_t)madt + 36) + HHDM_OFFSET;

    // Enable LAPIC (Spurious Interrupt Vector Register 0xF0)
    uint32_t val = *(uint32_t*)(lapic_base + 0xF0);
    val |= 0x1FF; // Enable bit 8, Vector 0xFF
    *(uint32_t*)(lapic_base + 0xF0) = val;
}

void apic_send_ipi(uint8_t lapic_id, uint32_t vector) {
    // ICR High: Destination
    *(uint32_t*)(lapic_base + 0x310) = (uint32_t)lapic_id << 24;
    // ICR Low: Vector and delivery mode
    *(uint32_t*)(lapic_base + 0x300) = vector;
}

void lapic_eoi(void) {
    *(uint32_t*)(lapic_base + 0xB0) = 0;
}

uint8_t lapic_get_id(void) {
    return (uint8_t)(*(uint32_t*)(lapic_base + 0x20) >> 24);
}
