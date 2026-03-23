# Carley Kernel - Arquitectura

## Visión General
Carley Kernel es un microkernel diseñado desde cero para x86_64, con un enfoque en la seguridad, la organización y la portabilidad hacia PCs y consolas de videojuegos.

## Principios de Diseño
- **Microkernel:** Solo las funciones esenciales (IPC, gestión de memoria, planificación) residen en el espacio del kernel.
- **Seguridad:** Aislamiento total entre servicios y procesos de usuario. Inspirado en la robustez de los sistemas modernos.
- **Modularidad:** Los controladores (drivers) y sistemas de archivos corren en el espacio de usuario como servidores independientes.

## Componentes del Kernel
1. **IPC (Inter-Process Communication):** El mecanismo central para la comunicación entre servicios.
2. **Gestión de Memoria:** Manejo de memoria física y paginación para x86_64.
3. **Planificador:** Gestión de hilos de ejecución.
4. **Syscalls:** Interfaz mínima para servicios de usuario.
