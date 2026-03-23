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
#include "vfs.h"
#include "initrd.h"
#include "drivers/video.h"

/* Marcadores del protocolo Limine */
__attribute__((used, section(".requests"))) volatile LIMINE_BASE_REVISION(2);
__attribute__((used, section(".requests"))) volatile struct limine_framebuffer_request framebuffer_request = { .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_memmap_request memmap_request = { .id = LIMINE_MEMMAP_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_hhdm_request hhdm_request = { .id = LIMINE_HHDM_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_kernel_address_request kernel_address_request = { .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0 };

/* Solicitud para modulos (Initrd) */
__attribute__((used, section(".requests"))) volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static void hlt(void) { for (;;) { __asm__("hlt"); } }

void ui_task(void) {
    ipc_msg_t msg;
    uint32_t x = 10, y = 150;

    video_draw_string("Carley Kernel v0.1", 10, 10, 0xFFFFFF);

    /* Intentar leer archivo del Initrd */
    vfs_node_t *node = vfs_open("welcome.txt");
    if (node) {
        char buffer[128];
        memset(buffer, 0, 128);
        vfs_read(node, 0, node->size, (uint8_t *)buffer);
        video_draw_string(buffer, 10, 110, 0xFFFF00);
    } else {
        video_draw_string("welcome.txt no encontrado en Initrd", 10, 110, 0xFF0000);
    }

    video_draw_string("Escribe algo:", 10, 130, 0xAAAAAA);

    for (;;) {
        if (ipc_recv(&msg) == 0) {
            if (msg.sender == 100) {
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

    pmm_init();
    vmm_init();
    kheap_init();
    gdt_init();
    idt_init();
    sched_init();
    syscall_init();
    vfs_init();

    /* Inicializar Initrd si existe un modulo cargado */
    if (module_request.response && module_request.response->module_count > 0) {
        struct limine_file *module = module_request.response->modules[0];
        vfs_root = initrd_init(module->address, module->size);
    }

    pit_init(100);
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) hlt();
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    video_init(fb);
    video_clear(0x1E1E1E);
    draw_logo();

    sched_create_task(ui_task, false);
    __asm__ volatile("sti");
    for (;;) hlt();
}
