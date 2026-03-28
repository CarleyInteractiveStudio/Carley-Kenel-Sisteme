#include "acpi.h"
#include "common/string.h"

static rsdp_t *rsdp = NULL;
static acpi_header_t *rsdt = NULL;

void acpi_init(void) {
    // Buscar RSDP en el primer Megabyte (BIOS convencional)
    for (uint32_t i = 0xE0000; i < 0xFFFFF; i += 16) {
        if (memcmp((void*)(uintptr_t)i, "RSD PTR ", 8) == 0) {
            rsdp = (rsdp_t*)(uintptr_t)i;
            break;
        }
    }

    if (rsdp) {
        rsdt = (acpi_header_t*)(uintptr_t)rsdp->rsdt_address;
    }
}

void *acpi_find_table(const char *sig) {
    if (!rsdt) return NULL;

    int entries = (rsdt->length - sizeof(acpi_header_t)) / 4;
    uint32_t *ptrs = (uint32_t*)((uintptr_t)rsdt + sizeof(acpi_header_t));

    for (int i = 0; i < entries; i++) {
        acpi_header_t *table = (acpi_header_t*)(uintptr_t)ptrs[i];
        if (memcmp(table->signature, sig, 4) == 0) return table;
    }
    return NULL;
}
