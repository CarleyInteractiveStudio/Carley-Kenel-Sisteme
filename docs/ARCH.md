# Carley Kernel - Arquitectura

## Visión General
Carley Kernel es un microkernel diseñado desde cero para x86_64, enfocado en seguridad extrema, modularidad y su uso en PCs y consolas de videojuegos.

## Principios de Diseño
- **Microkernel:** Carley OS sigue un diseño de microkernel. El Input Server (teclado) ya corre en espacio de usuario, y otros drivers seguirán.
- **Seguridad:** Sistema de **Capacidades (Capabilities)** que restringe el acceso a syscalls críticas (Disk, Hardware) por proceso. Aislamiento total Ring 3.
- **Modernidad:** Soporte para Multi-núcleo (SMP) y Enlazado Dinámico (Skeleton .so preparado).

## Componentes del Kernel

### 1. Gestión de Memoria (Implementado)
- **PMM:** Bitmap físico protegido por spinlocks (SMP Safe).
- **VMM:** Paginación x86_64 con CR3-switching, soporte de `mmap` para procesos aislados y protección por spinlocks (SMP Safe).
- **KHeap:** Gestión dinámica interna.

### 2. Gestión de CPU y Multitarea (Implementado)
- **GDT/TSS:** Estructuras GDT y TSS privadas por núcleo de CPU para garantizar aislamiento SMP.
- **Scheduler:** Round-Robin multi-core (SMP) con soporte SSE/FPU.
- **Seguridad:** Cada proceso tiene su propio mapa de páginas y stack de kernel privado.

### 3. IPC y Syscalls (Implementado)
- **Mensajería:** Colas privadas por tarea gestionadas de forma segura.
- **Syscalls:** Soporte para archivos, procesos, tiempo, audio y memoria compartida.

### 4. Almacenamiento (Implementado)
- **VFS:** Soporte multi-mount con resolución de rutas completa.
- **Sistemas:** Initrd (RO), RamFS (RW) y CarleyFS (IDE persistente).

### 5. Librerías y Multimedia (Implementado)
- **LibC:** Soporte para enlazado estático y preparación para enlazado dinámico.
- **Gráficos:** Servidor gráfico (Composer) con soporte para Sprites y Scroll.
- **Audio:** Driver SB16 para sonido 8-bit.
