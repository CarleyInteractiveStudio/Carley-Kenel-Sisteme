# Carley Kernel - Arquitectura

## Visión General
Carley Kernel es un microkernel diseñado desde cero para x86_64, con un enfoque en la seguridad, la organización y la portabilidad hacia PCs y consolas de videojuegos.

## Principios de Diseño
- **Microkernel:** Solo las funciones esenciales (IPC, gestión de memoria, planificación) residen en el espacio del kernel.
- **Seguridad:** Aislamiento total entre servicios y procesos de usuario. Inspirado en la robustez de los sistemas modernos.
- **Modularidad:** Los controladores (drivers) y sistemas de archivos corren en el espacio de usuario como servidores independientes.

## Componentes del Kernel

### 1. Gestión de Memoria (Implementado)
- **PMM (Physical Memory Manager):** Gestiona la memoria RAM física mediante un bitmap alineado a páginas (4KB).
- **VMM (Virtual Memory Manager):** Implementa paginación x86_64 de 4 niveles (PML4). Soporta el mapeo de memoria HHDM dinámico y la protección de memoria.
- **KHeap (Kernel Heap):** Gestor de memoria dinámica interna del kernel (`kmalloc`/`kfree`).

### 2. Gestión de CPU y Multitarea (Implementado)
- **GDT e IDT:** Configuración de descriptores y manejo de interrupciones/excepciones mediante un marco de pila robusto (`context_t`).
- **Planificador (Scheduler):** Implementa un algoritmo Round-Robin que conmuta tareas mediante interrupciones de hardware (PIT).
- **Context Switching:** Cambio de contexto completo que preserva todos los registros de la CPU, incluyendo punteros de pila dinámicos.

### 3. IPC (Inter-Process Communication) (Implementado)
- **Mensajería Síncrona:** Sistema de colas de mensajes que permite la comunicación fluida entre hilos de ejecución.
- *Nota:* La implementación actual utiliza una cola global simplificada para demostración en esta fase inicial.

### 4. Syscalls (Próximamente)
Interfaz mínima para que los servicios de usuario soliciten funciones al kernel.
