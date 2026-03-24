# Carley Kernel - Arquitectura

## Visión General
Carley Kernel es un microkernel diseñado desde cero para x86_64, con un enfoque en la seguridad extrema, la organización y la portabilidad hacia PCs y consolas de videojuegos.

## Principios de Diseño
- **Microkernel:** Las funciones esenciales residen en el kernel.
- **Seguridad:** Aislamiento Ring 3 con mapas de páginas únicos por proceso.
- **Modularidad:** Drivers y Sistemas de Archivos independientes.

## Componentes del Kernel

### 1. Gestión de Memoria (Implementado)
- **PMM:** Bitmap físico protegido por spinlocks.
- **VMM:** Paginación x86_64 con aislamiento total.
- **KHeap:** Gestión dinámica interna.

### 2. Gestión de CPU y Multitarea (Implementado)
- **Scheduler:** Round-Robin preentivo (100Hz).
- **TSS:** Cambio seguro de stacks.

### 3. IPC (Inter-Process Communication) (Implementado)
- **Mensajería:** Colas privadas por tarea.

### 4. Almacenamiento y VFS (Implementado)
- **VFS:** Capa de abstracción con soporte multimoizaje.
- **Initrd:** Disco en RAM de solo lectura para el arranque.
- **RamFS:** Disco en RAM escribible para datos temporales.
- **CarleyFS:** Sistema de archivos persistente para discos duros reales.

### 5. Drivers de Hardware (Implementado)
- **Video:** Dibujo gráfico y terminal con scroll.
- **Keyboard:** Driver PS/2 con buffer.
- **Disk (IDE/ATA):** Driver PIO para acceso a sectores de discos físicos.
