#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "common/limine.h"

/* Indicamos que estamos usando la revisión base de Limine.
   Eliminamos 'static' para que sean visibles globalmente en el ELF. */
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
    /* Verificar que la revisión base es soportada por el bootloader */
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hlt();
    }

    /* Verificar si obtuvimos el framebuffer */
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hlt();
    }

    /* Obtener el primer framebuffer */
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    /* Dibujar un patrón básico en pantalla (un cuadrado azul de 200x200) */
    uint32_t *fb_ptr = (uint32_t *)framebuffer->address;
    for (uint64_t i = 0; i < 200; i++) {
        for (uint64_t j = 0; j < 200; j++) {
            fb_ptr[i * (framebuffer->pitch / 4) + j] = 0x0000FF; // Color Azul (RGB)
        }
    }

    /* Bucle infinito de seguridad */
    hlt();
}
