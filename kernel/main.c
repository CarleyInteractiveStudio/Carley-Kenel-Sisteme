#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "common/limine.h"
#include "common/string.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "sched.h"
#include "ipc.h"
#include "syscall.h"
#include "drivers/video.h"

/* Marcadores del protocolo Limine */
__attribute__((used, section(".requests"))) volatile LIMINE_BASE_REVISION(2);
__attribute__((used, section(".requests"))) volatile struct limine_framebuffer_request framebuffer_request = { .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_memmap_request memmap_request = { .id = LIMINE_MEMMAP_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_hhdm_request hhdm_request = { .id = LIMINE_HHDM_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_kernel_address_request kernel_address_request = { .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0 };

static void hlt(void) { for (;;) { __asm__("hlt"); } }

/* Tarea de UI: Maneja el teclado y la pantalla */
void ui_task(void) {
    ipc_msg_t msg;
    uint32_t x = 10, y = 150;

    video_draw_string("Carley Kernel v0.1", 10, 10, 0xFFFFFF);
    video_draw_string("Escribe algo:", 10, 130, 0xAAAAAA);

    for (;;) {
        if (ipc_recv(&msg) == 0) {
            if (msg.sender == 100) { // Teclado
                char c = (char)msg.data[0];
                video_draw_char(c, x, y, 0x00FF00);
                x += 8;
                if (x > 300) { x = 10; y += 10; }
            }
        }
        sched_yield();
    }
}

void draw_logo(void) {
    video_draw_rect(50, 50, 40, 10, 0x3498DB);
    video_draw_rect(50, 50, 10, 40, 0x3498DB);
    video_draw_rect(50, 80, 40, 10, 0x3498DB);
    video_draw_rect(100, 50, 10, 40, 0xE74C3C);
    video_draw_rect(110, 65, 10, 10, 0xE74C3C);
    video_draw_rect(120, 50, 10, 15, 0xE74C3C);
    video_draw_rect(120, 75, 10, 15, 0xE74C3C);
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) hlt();

    /* 1. Inicializar Memoria y CPU básica */
    pmm_init();
    vmm_init();
    kheap_init();
    gdt_init();
    idt_init();

    /* 2. Inicializar Planificador y Syscalls */
    sched_init();
    syscall_init();

    /* 3. Inicializar Hardware (Timers, Pantalla) */
    pit_init(100);
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) hlt();
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    video_init(fb);
    video_clear(0x1E1E1E);
    draw_logo();

    /* 4. Crear tarea inicial */
    sched_create_task(ui_task, false);

    /* 5. Habilitar Interrupciones y saltar al primer hilo */
    __asm__ volatile("sti");

    /* Bucle de espera (el scheduler tomará el control al primer tick del PIT) */
    for (;;) {
        __asm__("hlt");
    }
}
