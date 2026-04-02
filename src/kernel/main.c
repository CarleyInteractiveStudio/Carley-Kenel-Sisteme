#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "boot_info.h"
#include "string.h"
#include "libc/stdio.h"
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
#include "drivers/acpi.h"
#include "config.h"
#include "elf.h"
#include "keyboard_buf.h"
#include "cpu.h"
#include "smp.h"
#include "drivers/apic.h"
#include "drivers/ahci.h"
#include "limine.h"

// Limine Requests
__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_smp_request smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;

extern void mouse_init(void);

static void hlt(void) { for (;;) { __asm__("hlt"); } }

void _start(void);
void draw_splash(void);

extern void serial_init();
extern void write_serial_string(const char* s);

void _start(void) {
    serial_init();
    write_serial_string("Kernel started via Limine!\n");
    cpu_enable_features();

    // Check Limine responses
    if (framebuffer_request.response == NULL || hhdm_request.response == NULL || memmap_request.response == NULL) {
        hlt();
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    uint64_t hhdm = hhdm_request.response->offset;

    // Inicializar video lo antes posible
    video_init_vbe((uintptr_t)fb->address, fb->width, fb->height);
    video_clear(0x0000FF); // Pantalla AZUL: el kernel ha empezado
    write_serial_string("Video initialized.\n");

    // Reutilizamos la estructura boot_info_t internamente
    boot_info_t binfo;
    binfo.framebuffer_address = (uintptr_t)fb->address - hhdm; // Base física
    binfo.screen_width = fb->width;
    binfo.screen_height = fb->height;
    binfo.memory_map_address = (uintptr_t)memmap_request.response->entries;
    binfo.memory_map_count = memmap_request.response->entry_count;
    binfo.rsdp_address = (rsdp_request.response) ? (uintptr_t)rsdp_request.response->address : 0;

    // Inicializar PMM con el mapa de Limine
    pmm_init_limine(memmap_request.response);
    write_serial_string("PMM initialized.\n");

    vmm_init(&binfo);
    write_serial_string("VMM initialized.\n");
    kheap_init();
    write_serial_string("Kheap initialized.\n");

    cpu_init_local(0);
    gdt_init();
    idt_init();

    // Inicializar ACPI
    acpi_init_custom(binfo.rsdp_address);

    apic_init();

    // Arrancar otros núcleos usando Limine
    if (smp_request.response) {
        smp_init_limine(smp_request.response);
        write_serial_string("SMP initialized via Limine.\n");
    }

    kbd_buf_init();
    sched_init();
    syscall_init();
    vfs_init();
    ide_init();
    ahci_init();

    // Cargar initrd desde Limine
    if (module_request.response) {
        initrd_load_limine(module_request.response);
        write_serial_string("Initrd loaded via Limine.\n");
    }

    vfs_mount(carleyfs_init());
    vfs_mount(ramfs_init());
    audio_init();

    pit_init(100);
    sched_create_task(audio_mixer_step, false);

    // Dibujar splash
    draw_splash();
    video_clear(0x1E1E1E);
    mouse_init();
    composer_start();

    // Cargar módulos
    elf_load("input.elf");
    elf_load("shell_gui.elf");

    __asm__ volatile("sti");
    for (;;) hlt();
}

void draw_splash(void) {
    video_clear(0x000000);
    char cpu_msg[64];
    sprintf(cpu_msg, "CARLEY OS - %d CORES DETECTED", smp_get_cpu_count());
    video_draw_string(cpu_msg, 270, 200, 0xFFFFFF);
    video_draw_rect(220, 230, 200, 10, 0x555555);
    video_draw_rect(220, 230, 50, 10, 0x3498DB);
}
