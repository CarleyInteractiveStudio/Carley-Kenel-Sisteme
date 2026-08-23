import "kernel/hal.dk"
import "kernel/memory.dk"
import "kernel/scheduler.dk"
import "sistema_operativo/fs.dk"
import "sistema_operativo/gui.dk"
import "project-K/lib/hardware/mouse.dk"
import "project-K/lib/hardware/keyboard.dk"
import "project-K/lib/hardware/speaker.dk"

# Punto de Entrada Principal de Carley Operating System (COS) en K

# 1. Inicializar Hardware y Subsistemas
hal_disable_interrupts()
memory_init()
scheduler_init()

# 2. Inicializar Dispositivos de Entrada y Sistema de Archivos
mouse_init()
fs_init()

# 3. Renderizar el Escritorio Básico y la GUI de COS
gui_draw_desktop()

# 4. Sonido de Inicio de Sistema
speaker_emit_tone(523)

hal_enable_interrupts()
