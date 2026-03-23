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

/* Marcadores del protocolo Limine */
__attribute__((used, section(".requests"))) volatile LIMINE_BASE_REVISION(2);
__attribute__((used, section(".requests"))) volatile struct limine_framebuffer_request framebuffer_request = { .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_memmap_request memmap_request = { .id = LIMINE_MEMMAP_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_hhdm_request hhdm_request = { .id = LIMINE_HHDM_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_kernel_address_request kernel_address_request = { .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0 };

static struct limine_framebuffer *fb;

/* Tarea 1: Productor de mensajes */
void task1(void) {
    uint32_t colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00};
    int i = 0;

    for (;;) {
        ipc_msg_t msg;
        msg.sender = 1;
        msg.type = 0xAA;
        msg.data[0] = colors[i];

        /* Enviar mensaje de color a task2 (ID=2) */
        ipc_send(2, &msg);

        i = (i + 1) % 4;

        /* Esperar un poco (ticks del PIT) para no saturar el IPC */
        uint64_t target_tick = pit_get_ticks() + 10;
        while (pit_get_ticks() < target_tick) sched_yield();
    }
}

/* Tarea 2: Consumidor de mensajes y Pintor */
void task2(void) {
    ipc_msg_t msg;
    uint32_t color = 0x000000;

    for (;;) {
        /* Recibir mensaje. Nota: en esta demo ID receptor = 0 (por simplificar ipc_recv) */
        // Pero hemos diseñado ipc_send para ID=2.
        // Corregimos ipc_recv para que acepte cualquier ID por ahora o sea para el actual.

        if (ipc_recv(&msg) == 0) {
            color = (uint32_t)msg.data[0];
        }

        /* Pintar cuadrado */
        for (uint64_t i = 110; i < 210; i++) {
            for (uint64_t j = 0; j < 100; j++) {
                ((uint32_t *)fb->address)[i * (fb->pitch / 4) + j] = color;
            }
        }
        sched_yield();
    }
}

/* Punto de entrada del kernel */
void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) { for (;;) __asm__("hlt"); }

    pmm_init();
    vmm_init();
    kheap_init();
    sched_init();
    gdt_init();
    idt_init();
    pit_init(100);

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) { for (;;) __asm__("hlt"); }
    fb = framebuffer_request.response->framebuffers[0];

    sched_create_task(task1); // Tarea 1
    sched_create_task(task2); // Tarea 2

    for (;;) {
        __asm__("hlt");
    }
}
