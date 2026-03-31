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

extern void mouse_init(void);

static void hlt(void) { for (;;) { __asm__("hlt"); } }

// SMP deshabilitado temporalmente para el cargador custom
// void ap_main(...) { ... }

void kmain(boot_info_t *boot_info);
void draw_splash(void);

extern void serial_init();
extern void write_serial_string(const char* s);

void kmain(boot_info_t *boot_info) {
    serial_init();
    write_serial_string("Kernel started!\n");
    cpu_enable_features();

    // Inicializar video lo antes posible para ver si arranca
    // Usamos el offset HHDM para acceder al framebuffer
    video_init_vbe(boot_info->framebuffer_address + HHDM_OFFSET, boot_info->screen_width, boot_info->screen_height);
    video_clear(0x0000FF); // Pantalla AZUL para depuración: el kernel ha empezado
    write_serial_string("Video initialized.\n");

    // Reemplazaremos pmm_init() para usar boot_info->memory_map_address
    // El mapa de memoria está en 0x9000, accesible via HHDM o identity
    pmm_init_custom(boot_info->memory_map_address + HHDM_OFFSET, boot_info->memory_map_count);
    write_serial_string("PMM initialized.\n");
    vmm_init(boot_info);
    write_serial_string("VMM initialized.\n");
    kheap_init();
    write_serial_string("Kheap initialized.\n");

    cpu_init_local(0);
    gdt_init();
    idt_init();

    // Inicializar ACPI con la dirección pasada por el cargador
    acpi_init_custom(boot_info->rsdp_address);

    apic_init();
    smp_init();

    kbd_buf_init();
    sched_init();
    syscall_init();
    vfs_init();
    ide_init();
    ahci_init();
    initrd_load_custom();
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

    // Cargar módulos desde el initrd custom
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
