#include "battery.h"
#include "acpi.h"
#include "kernel/cpu.h"

// Para VirtualBox y hardware real, la bateria suele estar en el EC (Embedded Controller)
// Por ahora simulamos una lectura base hasta implementar el AML Parser completo
void battery_init(void) {
    // Aqui habilitariamos el SCI (System Control Interrupt)
}

battery_info_t battery_get_info(void) {
    battery_info_t info;

    // Simulación para VirtualBox si no hay EC detectado
    info.status = 1;       // Cargando
    info.percentage = 85;  // Hardcoded para la demo visual inicial
    info.voltage = 12000;  // 12V

    return info;
}
