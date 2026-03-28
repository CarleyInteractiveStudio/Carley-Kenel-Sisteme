#include "ide.h"
#include "kernel/io.h"

/* Esperar a que el controlador esté listo */
static void ide_wait_ready(void) {
    while (inb(IDE_PRIMARY_BASE + 7) & 0x80); // Busy bit
    while (!(inb(IDE_PRIMARY_BASE + 7) & 0x40)); // Ready bit
}

void ide_init(void) {
    /* Detección simple del disco maestro primario */
    outb(IDE_PRIMARY_BASE + 6, 0xA0); // Seleccionar Master
}

int ide_read_sectors(uint32_t lba, uint8_t count, uint8_t *buffer) {
    ide_wait_ready();

    outb(IDE_PRIMARY_BASE + 2, count);
    outb(IDE_PRIMARY_BASE + 3, (uint8_t)lba);
    outb(IDE_PRIMARY_BASE + 4, (uint8_t)(lba >> 8));
    outb(IDE_PRIMARY_BASE + 5, (uint8_t)(lba >> 16));
    outb(IDE_PRIMARY_BASE + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IDE_PRIMARY_BASE + 7, 0x20); // Comando de lectura

    uint16_t *ptr = (uint16_t *)buffer;
    for (int i = 0; i < count; i++) {
        ide_wait_ready();
        /* Leer 256 words (512 bytes) por sector */
        for (int j = 0; j < 256; j++) {
            *ptr++ = inw(IDE_PRIMARY_BASE);
        }
    }
    return 0;
}

int ide_write_sectors(uint32_t lba, uint8_t count, uint8_t *buffer) {
    ide_wait_ready();

    outb(IDE_PRIMARY_BASE + 2, count);
    outb(IDE_PRIMARY_BASE + 3, (uint8_t)lba);
    outb(IDE_PRIMARY_BASE + 4, (uint8_t)(lba >> 8));
    outb(IDE_PRIMARY_BASE + 5, (uint8_t)(lba >> 16));
    outb(IDE_PRIMARY_BASE + 6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(IDE_PRIMARY_BASE + 7, 0x30); // Comando de escritura

    uint16_t *ptr = (uint16_t *)buffer;
    for (int i = 0; i < count; i++) {
        ide_wait_ready();
        for (int j = 0; j < 256; j++) {
            outw(IDE_PRIMARY_BASE, *ptr++);
        }
    }
    return 0;
}
