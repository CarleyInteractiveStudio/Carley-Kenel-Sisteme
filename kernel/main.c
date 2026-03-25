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
#include "ramfs.h"
#include "carleyfs.h"
#include "drivers/ide.h"
#include "drivers/video.h"
#include "drivers/audio.h"
#include "drivers/composer.h"
#include "elf.h"
#include "keyboard_buf.h"
#include "cpu.h"

extern void mouse_init(void);

__attribute__((used, section(".requests"))) volatile LIMINE_BASE_REVISION(2);
__attribute__((used, section(".requests"))) volatile struct limine_framebuffer_request framebuffer_request = { .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_memmap_request memmap_request = { .id = LIMINE_MEMMAP_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_hhdm_request hhdm_request = { .id = LIMINE_HHDM_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_kernel_address_request kernel_address_request = { .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_module_request module_request = { .id = LIMINE_MODULE_REQUEST, .revision = 0 };
__attribute__((used, section(".requests"))) volatile struct limine_smp_request smp_request = { .id = LIMINE_SMP_REQUEST, .revision = 0 };

static void hlt(void) { for (;;) { __asm__("hlt"); } }

void ap_main(struct limine_smp_info *info) {
    cpu_enable_features();
    cpu_init_local(info->lapic_id);
    gdt_init();
    idt_init();

    // El BSP ya creó una tarea inicial, nosotros creamos una idle para este AP
    // O simplemente entramos al scheduler con un contexto nulo para empezar.
    // Usaremos una técnica simple: crear una tarea idle infinita.
    sched_create_task(hlt, false);

    __asm__ volatile("sti");
    for (;;) hlt();
}

void draw_splash(void) {
    video_clear(0x000000);
    video_draw_string("CARLEY OS", 270, 200, 0xFFFFFF);
    video_draw_rect(220, 230, 200, 10, 0x555555);
    video_draw_rect(220, 230, 50, 10, 0x3498DB);
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) hlt();
    cpu_enable_features();

    pmm_init();
    vmm_init();
    kheap_init();

    /* Inicializar el núcleo actual (BSP) después del heap */
    cpu_init_local(0);
    gdt_init();
    idt_init();
    kbd_buf_init();
    sched_init();
    syscall_init();
    vfs_init();

    if (module_request.response && module_request.response->module_count > 0) {
        initrd_load_all(module_request.response);
    }

    ide_init();
    vfs_mount(carleyfs_init());
    vfs_mount(ramfs_init());
    audio_init();

    if (smp_request.response) {
        struct limine_smp_response *smp = smp_request.response;
        for (uint64_t i = 0; i < smp->cpu_count; i++) {
            struct limine_smp_info *cpu = smp->cpus[i];
            if (cpu->lapic_id != smp->bsp_lapic_id) cpu->goto_address = ap_main;
        }
    }

    pit_init(100);
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) hlt();
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    video_init(fb);
    draw_splash();
    video_clear(0x1E1E1E);
    mouse_init();
    composer_start();
    elf_load("input.elf");
    elf_load("shell.elf");

    __asm__ volatile("sti");
    for (;;) hlt();
}
