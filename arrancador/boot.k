import "project-K/lib/hardware/screen.dk"
import "project-K/lib/hardware/mouse.dk"
import "project-K/lib/hardware/speaker.dk"

# Arrancador nativo para CK Kernel / COS escrito en Lenguaje K

# 1. Limpiar pantalla e inicializar hardware en el arranque
screen_fill_color(31)

# 2. Imprimir mensaje de arranque del Kernel
screen_write_char(2, 1, 67, 31)  # C
screen_write_char(3, 1, 75, 31)  # K
screen_write_char(4, 1, 32, 31)  #
screen_write_char(5, 1, 66, 31)  # B
screen_write_char(6, 1, 79, 31)  # O
screen_write_char(7, 1, 79, 31)  # O
screen_write_char(8, 1, 84, 31)  # T

# 3. Inicializar dispositivos de entrada (Mouse / Teclado)
mouse_init()

# 4. Emitir tono de arranque exitoso
speaker_emit_tone(523)

# 5. Cargar e iniciar la ejecución del Kernel
screen_write_char(2, 3, 79, 30)  # O
screen_write_char(3, 3, 75, 30)  # K
