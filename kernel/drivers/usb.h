#ifndef USB_H
#define USB_H

#include <stdint.h>
#include "pci.h"

void usb_init(void);
void ehci_init(pci_device_t *pci);

#endif
