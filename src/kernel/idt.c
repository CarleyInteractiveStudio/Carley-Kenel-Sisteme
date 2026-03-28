#include "idt.h"
#include "string.h"
#include "io.h"
#include "sched.h"
#include "syscall.h"
#include "drivers/video.h"

static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr30();
extern void irq0(); extern void irq1(); extern void irq12(); // Mouse
extern void isr128();
extern void isr_generic();

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

    // Inicializar todas las entradas con un handler genérico
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint64_t)isr_generic, 0x08, 0x8E);
    }

    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x0);  outb(0xA1, 0x0);

    idt_set_gate(0, (uint64_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint64_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint64_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint64_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint64_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint64_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint64_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint64_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint64_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint64_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint64_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint64_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint64_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint64_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint64_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint64_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint64_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint64_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint64_t)isr20, 0x08, 0x8E);
    idt_set_gate(30, (uint64_t)isr30, 0x08, 0x8E);

    idt_set_gate(32, (uint64_t)irq0, 0x08, 0x8E); // Timer
    idt_set_gate(33, (uint64_t)irq1, 0x08, 0x8E); // KBD
    idt_set_gate(44, (uint64_t)irq12, 0x08, 0x8E); // Mouse (IRQ 12 -> 32 + 12 = 44)

    idt_set_gate(128, (uint64_t)isr128, 0x08, 0xEE);

    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

extern void timer_handler(void);
extern void keyboard_handler(void);
extern void mouse_handler(void);

static void pic_eoi(uint64_t int_no) {
    if (int_no >= 32 && int_no <= 47) {
        if (int_no >= 40) outb(0xA0, 0x20);
        outb(0x20, 0x20);
    }
}

context_t *irq_handler(uint64_t int_no, context_t *ctx) {
    if (int_no == 32) { timer_handler(); pic_eoi(int_no); return sched_schedule(ctx); }
    if (int_no == 33) { keyboard_handler(); pic_eoi(int_no); return ctx; }
    if (int_no == 44) { mouse_handler(); pic_eoi(int_no); return ctx; }
    if (int_no == 128) return syscall_handler(ctx);

    if (int_no < 32) {
        video_clear(0x880000);
        for (;;) { __asm__("hlt"); }
    }

    pic_eoi(int_no);
    return ctx;
}
