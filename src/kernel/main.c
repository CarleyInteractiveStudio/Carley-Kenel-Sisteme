#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "boot_info.h"
#include "string.h"
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

static void hlt(void) { for (;;) { __asm__("hlt"); } }

void kmain(boot_info_t *boot_info);

void draw_splash(void) {
    video_clear(0x000000);
    video_draw_string("CARLEY OS", 270, 200, 0xFFFFFF);
    video_draw_rect(220, 230, 200, 10, 0x555555);
    video_draw_rect(220, 230, 50, 10, 0x3498DB);
}

__attribute__((section(".text.head")))
void kmain(boot_info_t *boot_info) {
    cpu_enable_features();

    // Reemplazaremos pmm_init() para usar boot_info->memory_map_address
    pmm_init_custom(boot_info->memory_map_address, boot_info->memory_map_count);
    vmm_init(boot_info);
    kheap_init();

    cpu_init_local(0);
    gdt_init();
    idt_init();
    kbd_buf_init();
    sched_init();
    syscall_init();
    vfs_init();
    ide_init();
    initrd_load_custom();
    vfs_mount(carleyfs_init());
    vfs_mount(ramfs_init());
    audio_init();

    pit_init(100);
    sched_create_task(audio_mixer_step, false);

    // Inicializar video usando la dirección lineal de VBE
    video_init_vbe(boot_info->framebuffer_address, boot_info->screen_width, boot_info->screen_height);
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
