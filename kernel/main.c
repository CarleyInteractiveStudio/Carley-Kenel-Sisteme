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

/* Marcadores del protocolo Limine */
__attribute__((used, section(".requests"))) volatile LIMINE_BASE_REVISION(2);
__attribute__((used, section(".requests"))) volatile struct limine_framebuffer_request framebuffer_request = { .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_memmap_request memmap_request = { .id = LIMINE_MEMMAP_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_hhdm_request hhdm_request = { .id = LIMINE_HHDM_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_kernel_address_request kernel_address_request = { .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0 };

static struct limine_framebuffer *fb;

/* Función auxiliar para detener la CPU */
static void hlt(void) {
    for (;;) {
        __asm__("hlt");
    }
}

/* Wrapper para syscalls */
static inline void syscall_yield(void) { __asm__ volatile("int $0x80" : : "a"(SYS_YIELD)); }
static inline int syscall_ipc_send(uint64_t dest, void *msg) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_IPC_SEND), "D"(dest), "S"(msg)); return ret; }
static inline int syscall_ipc_recv(void *msg) { int ret; __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_IPC_RECV), "D"(msg)); return ret; }

/* Tarea 1: Productor (Simula espacio de usuario) */
void task1(void) {
    uint32_t colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00};
    int i = 0;
    for (;;) {
        ipc_msg_t msg;
        msg.sender = 1;
        msg.data[0] = colors[i];
        syscall_ipc_send(2, &msg);
        i = (i + 1) % 4;
        uint64_t target_tick = pit_get_ticks() + 10;
        while (pit_get_ticks() < target_tick) syscall_yield();
    }
}

/* Tarea 2: Consumidor (Simula espacio de usuario) */
void task2(void) {
    ipc_msg_t msg;
    uint32_t color = 0x000000;
    for (;;) {
        if (syscall_ipc_recv(&msg) == 0) {
            color = (uint32_t)msg.data[0];
        }
        for (uint64_t i = 110; i < 210; i++) {
            for (uint64_t j = 0; j < 100; j++) {
                ((uint32_t *)fb->address)[i * (fb->pitch / 4) + j] = color;
            }
        }
        syscall_yield();
    }
}

/* Punto de entrada del kernel */
void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) hlt();

    pmm_init();
    vmm_init();
    kheap_init();
    sched_init();
    syscall_init();
    gdt_init();
    idt_init();
    pit_init(100);

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) hlt();
    fb = framebuffer_request.response->framebuffers[0];

    /* Crear tareas simulando espacio de usuario (Ring 3) */
    sched_create_task(task1, true); // true = USER
    sched_create_task(task2, true); // true = USER

    for (;;) hlt();
}
