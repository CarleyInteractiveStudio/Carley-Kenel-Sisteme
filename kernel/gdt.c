#include "gdt.h"
#include "common/string.h"

#include "cpu.h"

extern void gdt_flush(uint64_t ptr);

static void gdt_set_entry(gdt_entry_t *gdt, int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index].base_low = (base & 0xFFFF);
    gdt[index].base_mid = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    gdt[index].limit_low = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[index].access = access;
}

static void gdt_set_tss(gdt_entry_t *gdt, int index, uint64_t base, uint32_t limit) {
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
    cpu_local_t *local = cpu_get_local();
    gdt_entry_t *gdt = local->gdt;
    tss_t *tss = &local->tss;

    memset(gdt, 0, sizeof(gdt_entry_t) * 7);

    gdt_set_entry(gdt, 0, 0, 0, 0, 0);                // Null
    gdt_set_entry(gdt, 1, 0, 0xFFFFFFFF, 0x9A, 0xA0); // Kernel Code
    gdt_set_entry(gdt, 2, 0, 0xFFFFFFFF, 0x92, 0xA0); // Kernel Data
    gdt_set_entry(gdt, 3, 0, 0xFFFFFFFF, 0xFA, 0xA0); // User Code
    gdt_set_entry(gdt, 4, 0, 0xFFFFFFFF, 0xF2, 0xA0); // User Data

    memset(tss, 0, sizeof(tss_t));
    tss->iopb_offset = sizeof(tss_t);
    gdt_set_tss(gdt, 5, (uint64_t)tss, sizeof(tss_t) - 1);

    local->gdt_ptr.limit = (sizeof(gdt_entry_t) * 7) - 1;
    local->gdt_ptr.base = (uint64_t)gdt;

    gdt_flush((uint64_t)&local->gdt_ptr);
    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}

void tss_set_rsp0(uint64_t rsp0) {
    cpu_local_t *local = cpu_get_local();
    local->tss.rsp0 = rsp0;
}
