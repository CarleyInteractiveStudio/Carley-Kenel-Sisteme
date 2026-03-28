#include <stdio.h>
#include <stdint.h>
#include <ipc.h>

/* Direcciones de puertos PS/2 */
#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64

/* Syscalls para I/O */
extern long syscall1(int num, long arg1);
#define SYS_IOPL 15

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void main(void) {
    /* Elevar privilegios de I/O para este proceso */
    syscall1(SYS_IOPL, 3);

    printf("Input Server: Iniciado (Microkernel mode).\n");

    while (1) {
        /* Polling simple del controlador de teclado PS/2 */
        if (inb(KBD_STATUS_PORT) & 1) {
            uint8_t scancode = inb(KBD_DATA_PORT);

            /* Enviar scancode al "Keyboard Buffer" o a quien lo necesite via IPC */
            ipc_msg_t msg;
            msg.sender = 500; // ID arbitrario para el Input Server
            msg.type = 1;     // Tipo: Evento de teclado
            msg.data[0] = scancode;

            /* Enviar mensaje al proceso 0 (que en este sistema redirige o es el buffer) */
            // En un sistema real, enviariamos esto a un "Session Manager"
            // Por ahora, lo imprimimos para depuracion
            if (scancode < 0x80) {
                // printf("Input Server: Scancode 0x%x detectado.\n", scancode);
            }
        }

        /* Ceder CPU para no bloquear */
        __asm__ volatile("int $0x80" : : "a"(0));
    }
}
