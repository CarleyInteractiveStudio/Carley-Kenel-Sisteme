#include "ahci.h"
#include "pci.h"
#include "vmm.h"
#include "string.h"
#include "drivers/video.h"
#include "config.h"

static hba_mem_t *hba_mem = NULL;

void ahci_init(void) {
    pci_device_t *dev = pci_find_device(0x01, 0x06); // Storage, SATA
    if (!dev) return;

    // BAR5 es la dirección base de los registros AHCI
    uintptr_t bar5 = pci_read_config(dev->bus, dev->slot, dev->func, 0x24);
    hba_mem = (hba_mem_t *)(bar5 + HHDM_OFFSET);

    // Habilitar AHCI (GHC.AE = 1)
    hba_mem->ghc |= (1 << 31);

    // Buscar puertos activos
    uint32_t pi = hba_mem->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            hba_port_t *port = &hba_mem->ports[i];
            uint32_t ssts = port->ssts;
            uint8_t det = ssts & 0x0F;
            uint8_t ipm = (ssts >> 8) & 0x0F;

            if (det == HBA_PORT_DET_PRESENT && ipm == HBA_PORT_IPM_ACTIVE) {
                // Puerto con dispositivo SATA encontrado
                // Aquí se inicializarían las Command Lists, etc.
            }
        }
    }
}
