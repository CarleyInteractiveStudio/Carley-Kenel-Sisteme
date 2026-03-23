#include "idt.h"
#include "common/string.h"
#include "io.h"
#include "sched.h"
#include "syscall.h"

static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

/* ISRs externas en isr.s */
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr10(); extern void isr11(); extern void isr12();
extern void isr13(); extern void isr14();
extern void irq0();
extern void isr128();

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

    /* Mapeo de excepciones con sus propios manejadores para mejor depuración */
    idt_set_gate(0, (uint64_t)isr0, 0x08, 0x8E);   // Divide-by-zero
    idt_set_gate(1, (uint64_t)isr1, 0x08, 0x8E);   // Debug
    idt_set_gate(2, (uint64_t)isr2, 0x08, 0x8E);   // NMI
    idt_set_gate(3, (uint64_t)isr3, 0x08, 0x8E);   // Breakpoint
    idt_set_gate(4, (uint64_t)isr4, 0x08, 0x8E);   // Overflow
    idt_set_gate(5, (uint64_t)isr5, 0x08, 0x8E);   // Bound Range Exceeded
    idt_set_gate(6, (uint64_t)isr6, 0x08, 0x8E);   // Invalid Opcode
    idt_set_gate(7, (uint64_t)isr7, 0x08, 0x8E);   // Device Not Available
    idt_set_gate(8, (uint64_t)isr8, 0x08, 0x8E);   // Double Fault
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E); // Invalid TSS
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E); // Segment Not Present
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E); // Stack-Segment Fault
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E); // General Protection Fault (GPF)
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E); // Page Fault (PF)

    /* Mapeo de IRQs */
    idt_set_gate(32, (uint64_t)irq0, 0x08, 0x8E);

    /* Syscalls (int $0x80) */
    idt_set_gate(128, (uint64_t)isr128, 0x08, 0xEE);

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

    if (int_no == 128) {
        return syscall_handler(ctx);
    }

    if (int_no < 32) {
        /* Excepción Crítica Detectada */
        /* En un microkernel real, intentaríamos matar la tarea actual si es de usuario */
        for (;;) { __asm__("hlt"); }
    }

    pic_eoi(int_no);
    return ctx;
}
