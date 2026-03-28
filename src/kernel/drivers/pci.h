#ifndef PCI_H
#define PCI_H

#include <stdint.h>

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint32_t bar0;
} pci_device_t;

void pci_init(void);
pci_device_t *pci_find_device(uint8_t class, uint8_t subclass);

#endif
