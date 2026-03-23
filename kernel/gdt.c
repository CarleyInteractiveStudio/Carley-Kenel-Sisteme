#include "gdt.h"
#include "common/string.h"

static gdt_entry_t gdt[5];
static gdt_ptr_t gdt_ptr;

/* Función en ensamblador para cargar la GDT */
extern void gdt_flush(uint64_t ptr);

void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index].base_low = (base & 0xFFFF);
    gdt[index].base_mid = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    gdt[index].limit_low = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[index].access = access;
}

void gdt_init(void) {
    /* Entrada 0: Nula (Obligatoria) */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* Entrada 1: Código Kernel (64 bits) */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xA0);

    /* Entrada 2: Datos Kernel */
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xA0);

    /* Entrada 3: Código Usuario */
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xA0);

    /* Entrada 4: Datos Usuario */
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xA0);

    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    gdt_flush((uint64_t)&gdt_ptr);
}
