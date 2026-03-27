#include "kernel/io.h"
#include "kernel/ipc.h"
#include "kernel/sched.h"
#include "drivers/video.h"

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];
static int32_t mouse_x = 320;
static int32_t mouse_y = 240;

/* Esperar a que el controlador PS/2 esté listo para escribir */
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) if ((inb(0x64) & 1) == 1) return;
    } else {
        while (timeout--) if ((inb(0x64) & 2) == 0) return;
    }
}

static void mouse_write(uint8_t a) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, a);
}

static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init(void) {
    uint8_t status;

    /* Habilitar el segundo canal PS/2 (Mouse) */
    mouse_wait(1);
    outb(0x64, 0xA8);

    /* Habilitar interrupciones */
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    /* Comandos de inicialización del mouse */
    mouse_write(0xF6); // Default settings
    mouse_read();
    mouse_write(0xF4); // Enable streaming
    mouse_read();
}

void mouse_handler(void) {
    uint8_t status = inb(0x64);
    if (!(status & 1) || !(status & 0x20)) return;

    mouse_byte[mouse_cycle++] = inb(0x60);

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        /* Procesar coordenadas relativas */
        int32_t rel_x = mouse_byte[1];
        int32_t rel_y = mouse_byte[2];

        if (mouse_byte[0] & 0x10) rel_x |= 0xFFFFFF00;
        if (mouse_byte[0] & 0x20) rel_y |= 0xFFFFFF00;

        mouse_x += rel_x;
        mouse_y -= rel_y; // Eje Y invertido en PS/2

        /* Limites de pantalla (800x600 asumiendo resolucion estandar) */
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_x > 790) mouse_x = 790;
        if (mouse_y > 590) mouse_y = 590;

        /* Enviar evento al Composer (ID 1) */
        ipc_msg_t msg;
        msg.sender = 502; // Mouse driver ID
        msg.type = 20;    // MOUSE_EVENT
        msg.data[0] = (uint32_t)mouse_x;
        msg.data[1] = (uint32_t)mouse_y;
        msg.data[2] = (mouse_byte[0] & 1); // Botón izquierdo
        ipc_send(1, &msg);
    }
}
