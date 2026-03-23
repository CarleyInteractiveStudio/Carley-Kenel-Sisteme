# Carley Kernel - Arquitectura

## Visión General
Carley Kernel es un microkernel diseñado desde cero para x86_64, con un enfoque en la seguridad extrema, la organización y la portabilidad hacia PCs y consolas de videojuegos.

## Principios de Diseño
- **Microkernel:** Las funciones esenciales (IPC, gestión de memoria, planificación) residen en el espacio del kernel.
- **Seguridad:** Aislamiento total entre servicios y procesos de usuario. El kernel NO es accesible desde Ring 3.
- **Sincronización:** Uso de Spinlocks atómicos para proteger estructuras de datos críticas.
- **Modularidad:** Los controladores y sistemas de archivos son componentes independientes.

## Componentes del Kernel

### 1. Gestión de Memoria (Implementado)
- **PMM (Physical Memory Manager):** Gestiona la memoria RAM física mediante un bitmap protegido por spinlocks.
- **VMM (Virtual Memory Manager):** Implementa paginación x86_64 de 4 niveles (PML4).
- **KHeap (Kernel Heap):** Gestor de memoria dinámica interna del kernel.

### 2. Gestión de CPU y Multitarea (Implementado)
- **GDT e IDT:** Configuración de descriptores y manejo de interrupciones/excepciones.
- **TSS (Task State Segment):** Permite el cambio seguro de pila al entrar al kernel.
- **Planificador (Scheduler):** Algoritmo Round-Robin que conmuta tareas mediante el PIT.

### 3. IPC (Inter-Process Communication) (Implementado)
- **Mensajería Síncrona:** Sistema de colas de mensajes protegido por spinlocks.

### 4. Almacenamiento y VFS (Implementado)
- **VFS (Virtual File System):** Capa de abstracción para manejar archivos y directorios.
- **Initrd (RAMDisk):** Sistema de archivos de solo lectura cargado por el bootloader que contiene los archivos iniciales del sistema.
- **Syscalls de Archivo:** Interfaz para abrir (`SYS_OPEN`) y leer (`SYS_READ`) archivos desde espacio de usuario.

### 5. Drivers y Servidores (Implementado)
- **Video Driver:** Dibujo de píxeles, rectángulos y texto.
- **Keyboard Driver:** Maneja IRQ1 y traduce scancodes a eventos ASCII.
- **UI Server:** Gestiona la interacción interactiva y muestra mensajes del disco.
