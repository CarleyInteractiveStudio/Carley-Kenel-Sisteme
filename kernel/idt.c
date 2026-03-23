#include "idt.h"
#include "common/string.h"
#include "io.h"
#include "sched.h"

static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

/* ISRs externas */
extern void isr0(); extern void isr13(); extern void isr14();
extern void irq0();

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].attributes = flags;
    idt[num].reserved = 0;
}

void idt_init(void) {
    idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr.base = (uint64_t)&idt;

    memset(&idt, 0, sizeof(idt_entry_t) * 256);

    /* Remapear el PIC */
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x0);  outb(0xA1, 0x0);

    /* Mapeo de excepciones mínimas */
    for (int i = 0; i < 32; i++) idt_set_gate(i, (uint64_t)isr0, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E);

    /* Mapeo de IRQs */
    idt_set_gate(32, (uint64_t)irq0, 0x08, 0x8E); // IRQ0 -> Timer

    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

extern void timer_handler(void);

static void pic_eoi(uint64_t int_no) {
    if (int_no >= 32 && int_no <= 47) {
        if (int_no >= 40) outb(0xA0, 0x20);
        outb(0x20, 0x20);
    }
}

context_t *irq_handler(uint64_t int_no, context_t *ctx) {
    if (int_no == 32) {
        timer_handler();
        pic_eoi(int_no);
        return sched_schedule(ctx);
    }

    if (int_no < 32) {
        for (;;) { __asm__("hlt"); }
    }

    pic_eoi(int_no);
    return ctx;
}
