import "kernel/hal.dk"
import "kernel/memory.dk"
import "kernel/scheduler.dk"
import "project-K/lib/hardware/screen.dk"
import "project-K/lib/hardware/speaker.dk"

# Inicialización Principal del Kernel CK (Carley Kernel) en Lenguaje K

hal_disable_interrupts()

# Inicializar gestor de memoria y planificador
memory_init()
scheduler_init()

# Crear tareas iniciales del kernel
p1 = create_task(1, 10)
p2 = create_task(2, 5)

# Pantalla de confirmación de arranque del Kernel
screen_fill_color(16) # Azul oscuro kernel

# Escribir "CK KERNEL READY"
screen_write_char(2, 2, 67, 30)  # C
screen_write_char(3, 2, 75, 30)  # K
screen_write_char(4, 2, 32, 30)  #
screen_write_char(5, 2, 75, 30)  # K
screen_write_char(6, 2, 69, 30)  # E
screen_write_char(7, 2, 82, 30)  # R
screen_write_char(8, 2, 78, 30)  # N
screen_write_char(9, 2, 69, 30)  # E
screen_write_char(10, 2, 76, 30) # L

hal_enable_interrupts()
speaker_emit_tone(600)
