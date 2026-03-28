# Carley Kernel - Objetivos y Fases

## Objetivos
1. Crear un microkernel independiente de Linux y otros sistemas existentes.
2. Soportar arquitecturas modernas como x86_64 y, en el futuro, ARM para consolas portátiles.
3. Desarrollar un ecosistema de drivers modular y seguro.
4. Facilitar la creación de sistemas operativos tanto para PC como para consolas (tipo PlayStation o Steam Deck).

## Fases de Desarrollo
### Fase 1: Cimientos y Arranque
- Configuración de Limine como cargador de arranque.
- Kernel entry point y salida básica de texto.
- Manejo inicial de la CPU (GDT/IDT).

### Fase 2: El Corazón del Microkernel
- Gestores de memoria física y virtual.
- Implementación de un sistema de IPC robusto.
- Planificación de hilos y procesos.

### Fase 3: Servicios del Sistema
- Drivers de entrada (teclado, ratón) en espacio de usuario.
- Driver gráfico (framebuffer) para la interfaz de Carley Kernel.
- Sistema de archivos para almacenamiento.

### Fase 4: Aplicaciones y Shell
- Librería estándar mínima (libc).
- Cargador de ejecutables.
- Interfaz de usuario básica.
