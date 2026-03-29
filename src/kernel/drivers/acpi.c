#include "acpi.h"
#include "string.h"
#include "pmm.h"
#include "config.h"

static rsdp_t *rsdp = NULL;
static acpi_header_t *rsdt = NULL;

void acpi_init(void) {
    // Buscar RSDP en el primer Megabyte (BIOS convencional)
    // Accedemos via HHDM
    for (uintptr_t i = 0xE0000; i < 0xFFFFF; i += 16) {
        if (memcmp((void*)(i + HHDM_OFFSET), "RSD PTR ", 8) == 0) {
            rsdp = (rsdp_t*)(i + HHDM_OFFSET);
            break;
        }
    }

    if (rsdp) {
        rsdt = (acpi_header_t*)((uintptr_t)rsdp->rsdt_address + HHDM_OFFSET);
    }
}

void *acpi_find_table(const char *sig) {
    if (!rsdt) return NULL;

    int entries = (rsdt->length - sizeof(acpi_header_t)) / 4;
    uint32_t *ptrs = (uint32_t*)((uintptr_t)rsdt + sizeof(acpi_header_t));

    for (int i = 0; i < entries; i++) {
        acpi_header_t *table = (acpi_header_t*)((uintptr_t)ptrs[i] + HHDM_OFFSET);
        if (memcmp(table->signature, sig, 4) == 0) return table;
    }
    return NULL;
}
