#ifndef APIC_H
#define APIC_H

#include <stdint.h>

void apic_init(void);
void apic_send_ipi(uint8_t lapic_id, uint32_t vector);
void lapic_eoi(void);
uint8_t lapic_get_id(void);

#endif
