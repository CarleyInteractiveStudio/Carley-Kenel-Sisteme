#include "usb.h"
#include "pci.h"
#include "string.h"

void usb_init(void) {
    // Buscar controladores EHCI (USB 2.0)
    pci_device_t *ehci = pci_find_device(0x0C, 0x03); // Class 0C, Subclass 03
    if (ehci && ehci->prog_if == 0x20) {
        ehci_init(ehci);
    }
}

void ehci_init(pci_device_t *pci) {
    // Inicialización básica del controlador EHCI
    uint32_t base = pci->bar0 & 0xFFFFFFF0;
    (void)base; // Prevent unused warning, placeholder for EHCI register access
    // (Lógica de registros EHCI omitida por brevedad)
}
