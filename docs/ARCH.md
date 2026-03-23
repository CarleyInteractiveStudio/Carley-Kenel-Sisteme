# Carley Kernel - Arquitectura

## Visión General
Carley Kernel es un microkernel diseñado desde cero para x86_64, con un enfoque en la seguridad, la organización y la portabilidad hacia PCs y consolas de videojuegos.

## Principios de Diseño
- **Microkernel:** Solo las funciones esenciales (IPC, gestión de memoria, planificación) residen en el espacio del kernel.
- **Seguridad:** Aislamiento total entre servicios y procesos de usuario. Inspirado en la robustez de los sistemas modernos.
- **Modularidad:** Los controladores (drivers) y sistemas de archivos corren en el espacio de usuario como servidores independientes.

## Componentes del Kernel

### 1. Gestión de Memoria (Implementado)
- **PMM (Physical Memory Manager):** Gestiona la memoria RAM física mediante un bitmap. Permite reservar páginas de 4KB.
- **VMM (Virtual Memory Manager):** Implementa paginación x86_64 de 4 niveles (PML4). Soporta el mapeo de memoria HHDM y la protección de memoria.
- **KHeap (Kernel Heap):** Gestor de memoria dinámica interna del kernel (`kmalloc`/`kfree`) para estructuras de datos flexibles.

### 2. IPC (Inter-Process Communication) (Próximamente)
El mecanismo central para la comunicación entre servicios.

### 3. Planificador (Próximamente)
Gestión de hilos de ejecución y cambio de contexto.

### 4. Syscalls (Próximamente)
Interfaz mínima para servicios de usuario.
