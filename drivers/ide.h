#ifndef IDE_H
#define IDE_H

#include <stdint.h>

#define IDE_PRIMARY_BASE   0x1F0
#define IDE_PRIMARY_CTRL   0x3F6

void ide_init(void);

/* Lee 'count' sectores (512 bytes c/u) del LBA dado en el buffer */
int ide_read_sectors(uint32_t lba, uint8_t count, uint8_t *buffer);

/* Escribe 'count' sectores desde el buffer al disco */
int ide_write_sectors(uint32_t lba, uint8_t count, uint8_t *buffer);

#endif
