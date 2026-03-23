#include "kernel/io.h"
#include "kernel/ipc.h"
#include "common/string.h"

/* Tabla simple de traducción de Scancode a ASCII (Set 1) */
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/* Manejador de la interrupción del teclado (IRQ1) */
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    /* Solo nos interesan las pulsaciones (no cuando se suelta la tecla, scancode < 0x80) */
    if (scancode < 0x80) {
        char key = scancode_to_ascii[scancode];
        if (key != 0) {
            /* Enviar evento de tecla al sistema vía IPC */
            ipc_msg_t msg;
            msg.sender = 100; // ID reservado para el driver de teclado
            msg.type = 0x11;  // Tipo: Tecla presionada
            msg.data[0] = (uint64_t)key;
            ipc_send(0, &msg); // Enviar a la cola global por ahora
        }
    }
}
