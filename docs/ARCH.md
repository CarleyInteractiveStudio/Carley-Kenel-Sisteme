# Carley Kernel - Arquitectura

## Visión General
Carley Kernel es un microkernel diseñado desde cero para x86_64, con un enfoque en la seguridad extrema, la organización y la portabilidad hacia PCs y consolas de videojuegos.

## Principios de Diseño
- **Microkernel:** Las funciones esenciales (IPC, gestión de memoria, planificación) residen en el espacio del kernel.
- **Seguridad:** Aislamiento total entre servicios y procesos de usuario. El kernel NO es accesible desde Ring 3 (sin bit PTE_USER).
- **Sincronización:** Uso de Spinlocks atómicos para proteger estructuras de datos críticas en entornos multitarea.
- **Modularidad:** Los controladores (drivers) y sistemas de archivos corren como servidores independientes.

## Componentes del Kernel

### 1. Gestión de Memoria (Implementado)
- **PMM (Physical Memory Manager):** Gestiona la memoria RAM física mediante un bitmap protegido por spinlocks.
- **VMM (Virtual Memory Manager):** Implementa paginación x86_64 de 4 niveles (PML4) con aislamiento estricto de privilegios.
- **KHeap (Kernel Heap):** Gestor de memoria dinámica interna del kernel protegido por spinlocks.

### 2. Gestión de CPU y Multitarea (Implementado)
- **GDT e IDT:** Configuración de descriptores y manejo de interrupciones/excepciones.
- **TSS (Task State Segment):** Permite el cambio seguro de pila al entrar al kernel desde el espacio de usuario.
- **Planificador (Scheduler):** Algoritmo Round-Robin que conmuta tareas mediante el PIT (IRQ0).

### 3. IPC (Inter-Process Communication) (Implementado)
- **Mensajería Síncrona:** Sistema de colas de mensajes protegido por spinlocks que permite la comunicación entre hilos de ejecución.

### 4. Syscalls (Implementado)
- **Interfaz Ring 3:** Los procesos de usuario se comunican con el kernel mediante `int $0x80`.

### 5. Drivers y Servidores (Implementado)
- **Video Driver:** Dibujo de píxeles, rectángulos y texto. Renderiza el logo de Carley Kernel.
- **Keyboard Driver:** Maneja IRQ1 y traduce scancodes a eventos que se envían vía IPC.
- **UI Server:** Gestiona la interacción interactiva entre el teclado y la pantalla.
