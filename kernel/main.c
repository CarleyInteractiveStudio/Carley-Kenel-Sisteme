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
#include "drivers/audio.h" // Nuevo
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

static void hlt(void) { for (;;) { __asm__("hlt"); } }

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) hlt();
    cpu_enable_features();
    pmm_init();
    vmm_init();
    kheap_init();
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

    /* Inicializar Audio */
    audio_init();

    pit_init(100);
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) hlt();
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    video_init(fb);
    video_clear(0x1E1E1E);

    mouse_init();
    composer_start();
    elf_load("shell.elf");

    __asm__ volatile("sti");
    for (;;) hlt();
}
