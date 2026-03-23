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
- **TSS (Task State Segment):** Permite el cambio seguro de pila al entrar al kernel desde el espacio de usuario. El kernel actualiza el campo `rsp0` en cada cambio de contexto.
- **Planificador (Scheduler):** Implementa un algoritmo Round-Robin que conmuta tareas mediante interrupciones de hardware (PIT). Soporta tanto tareas de Kernel (Ring 0) como de Usuario (Ring 3).

### 3. IPC (Inter-Process Communication) (Implementado)
- **Mensajería Síncrona:** Sistema de colas de mensajes que permite la comunicación fluida entre hilos de ejecución.

### 4. Syscalls (Implementado)
- **Interfaz Ring 3:** Los procesos de usuario se comunican con el kernel mediante la interrupción `int $0x80`.
- **Llamadas Disponibles:** `SYS_YIELD`, `SYS_IPC_SEND`, `SYS_IPC_RECV`.
- **Aislamiento:** Las tareas en Ring 3 están aisladas mediante privilegios de CPU y permisos de página (U/S bit), garantizando la seguridad del sistema.

### 5. Drivers (Próximamente)
Controladores de hardware corriendo como servicios de usuario.
