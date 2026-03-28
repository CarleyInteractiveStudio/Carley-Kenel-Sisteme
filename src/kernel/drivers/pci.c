#include "pci.h"
#include "cpu.h"
#include "string.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static pci_device_t pci_devices[64];
static int pci_count = 0;

static uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_init(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_read_config(bus, slot, func, 0x00);
                if ((vendor & 0xFFFF) == 0xFFFF) continue;

                uint32_t class_data = pci_read_config(bus, slot, func, 0x08);
                pci_devices[pci_count].bus = bus;
                pci_devices[pci_count].slot = slot;
                pci_devices[pci_count].func = func;
                pci_devices[pci_count].vendor_id = vendor & 0xFFFF;
                pci_devices[pci_count].device_id = vendor >> 16;
                pci_devices[pci_count].class_code = (class_data >> 24) & 0xFF;
                pci_devices[pci_count].subclass = (class_data >> 16) & 0xFF;
                pci_devices[pci_count].prog_if = (class_data >> 8) & 0xFF;
                pci_devices[pci_count].bar0 = pci_read_config(bus, slot, func, 0x10);

                pci_count++;
                if (pci_count >= 64) return;
            }
        }
    }
}

pci_device_t *pci_find_device(uint8_t class, uint8_t subclass) {
    for (int i = 0; i < pci_count; i++) {
        if (pci_devices[i].class_code == class && pci_devices[i].subclass == subclass) {
            return &pci_devices[i];
        }
    }
    return NULL;
}
