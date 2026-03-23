#include "gdt.h"
#include "common/string.h"

static gdt_entry_t gdt[7];
static gdt_ptr_t gdt_ptr;
static tss_t tss;

extern void gdt_flush(uint64_t ptr);

void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index].base_low = (base & 0xFFFF);
    gdt[index].base_mid = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    gdt[index].limit_low = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[index].access = access;
}

void gdt_set_tss(int index, uint64_t base, uint32_t limit) {
    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_mid = (base >> 16) & 0xFF;
    gdt[index].access = 0x89;
    gdt[index].granularity = ((limit >> 16) & 0x0F);
    gdt[index].base_high = (base >> 24) & 0xFF;

    uint32_t *upper = (uint32_t *)&gdt[index + 1];
    *upper = (base >> 32) & 0xFFFFFFFF;
    *(upper + 1) = 0;
}

void gdt_init(void) {
    memset(&gdt, 0, sizeof(gdt));

    gdt_set_entry(0, 0, 0, 0, 0);                // Null
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xA0); // Kernel Code
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xA0); // Kernel Data
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xA0); // User Code
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xA0); // User Data

    memset(&tss, 0, sizeof(tss));
    tss.iopb_offset = sizeof(tss);
    gdt_set_tss(5, (uint64_t)&tss, sizeof(tss) - 1);

    gdt_ptr.limit = (sizeof(gdt_entry_t) * 7) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    gdt_flush((uint64_t)&gdt_ptr);
    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}

/* Función para actualizar el stack de kernel en el TSS durante el cambio de contexto */
void tss_set_rsp0(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}
