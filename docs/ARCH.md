# Carley Kernel - Arquitectura

## Visión General
Carley Kernel es un microkernel diseñado desde cero para x86_64, enfocado en seguridad, modularidad y su uso en PCs y consolas de videojuegos.

## Principios de Diseño
- **Microkernel:** Servicios esenciales en el kernel, drivers en el espacio de usuario (en transición).
- **Seguridad:** Aislamiento total Ring 3 con handles para recursos del sistema.
- **Multimedia:** Soporte nativo para gráficos acelerados por software (Composer) y audio (SB16).

## Componentes del Kernel

### 1. Gestión de Memoria (Implementado)
- **PMM:** Bitmap físico protegido por spinlocks.
- **VMM:** Paginación x86_64 con CR3-switching por proceso.
- **KHeap:** Gestión dinámica interna.

### 2. Gestión de CPU y Multitarea (Implementado)
- **Scheduler:** Round-Robin preentivo con soporte SSE/FPU para cálculos de punto flotante.
- **TSS:** Manejo seguro de pilas durante interrupciones.

### 3. IPC y Syscalls (Implementado)
- **Mensajería:** Colas privadas por tarea con soporte para paso de datos multimedia.
- **Interfaz:** Syscalls vía `int $0x80` para archivos, procesos, tiempo y audio.

### 4. Almacenamiento (Implementado)
- **VFS:** Soporte para múltiples montajes.
- **RamFS & Initrd:** Almacenamiento volátil de alto rendimiento.
- **CarleyFS:** Sistema de archivos persistente para discos IDE.

### 5. Drivers y Multimedia (Implementado)
- **Video & Composer:** Servidor gráfico con soporte para sprites y scroll.
- **Audio (SB16):** Driver para reproducción de audio PCM de 8 bits.
- **Input:** Drivers de teclado y mouse PS/2.
- **RTC:** Reloj de tiempo real para gestión de fecha/hora.
