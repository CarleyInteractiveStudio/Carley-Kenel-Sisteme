#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "common/limine.h"
#include "common/string.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"

/* Indicamos que estamos usando la revisión base de Limine */
__attribute__((used, section(".requests")))
volatile LIMINE_BASE_REVISION(2);

/* Solicitud para el framebuffer de Limine */
__attribute__((used, section(".requests")))
volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

/* Función auxiliar para detener la CPU */
static void hlt(void) {
    for (;;) {
        __asm__("hlt");
    }
}

/* Punto de entrada del kernel */
void kmain(void) {
    /* Verificar revisión base */
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hlt();
    }

    /* Inicializar PMM, VMM y el Montón (Heap) */
    pmm_init();
    vmm_init();
    kheap_init();

    /* Obtener el primer framebuffer */
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hlt();
    }
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    /* Prueba del Montón: Reservar memoria dinámicamente */
    uint32_t *color_ptr = kmalloc(sizeof(uint32_t));
    if (color_ptr) {
        *color_ptr = 0xFFFF00; // Amarillo si el montón funciona
    } else {
        hlt(); // Fallo crítico si no hay montón
    }

    /* Dibujar un patrón básico en pantalla */
    uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
    for (uint64_t i = 0; i < 200; i++) {
        for (uint64_t j = 0; j < 200; j++) {
            fb_ptr[i * (framebuffer->pitch / 4) + j] = *color_ptr;
        }
    }

    /* Liberar memoria de prueba */
    kfree(color_ptr);

    /* Bucle infinito de seguridad */
    hlt();
}
