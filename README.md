# 🌌 Carley OS

Carley OS es un sistema operativo moderno, seguro y visualmente atractivo diseñado para PCs y consolas. Es un proyecto desarrollado desde cero, independiente de Linux, enfocado en gaming, programación y el alto rendimiento.

<p align="center">
  <b>Estado Actual: 85% - Camino a la Versión 1.0</b>
</p>

## ✨ Características Principales

*   **Microkernel x86_64:** Arquitectura basada en el paso de mensajes (IPC) sincrónico y servicios en espacio de usuario.
*   **Symmetric Multiprocessing (SMP):** Soporte real para múltiples núcleos de CPU con planificación Round-Robin.
*   **Interfaz de Usuario "Apple-Style":** El **Composer** gestiona ventanas con efectos de cristal (Alpha Blending) y un Dashboard central.
*   **Seguridad por Capacidades:** Sistema de permisos granulados que restringe el acceso al hardware, E/S y disco por proceso.
*   **Almacenamiento Persistente:** **CarleyFS**, un sistema de archivos persistente diseñado para discos IDE/ATA.
*   **Entorno de Desarrollo Integrado:**
    *   Soporte nativo para C y C++ (con runtime mínimo).
    *   **Carley Script:** Intérprete de scripts integrado para automatización y apps rápidas.
    *   Cargador ELF dinámico con soporte para librerías compartidas (`libc.so`).
*   **Multimedia:** Mezclador de audio de 4 canales para hardware SoundBlaster 16.

## 🏗️ Arquitectura Técnica

Carley OS sigue una filosofía de microkernel estricto:
- **Kernel:** Gestiona memoria (PMM/VMM), hilos, IPC y el despacho de interrupciones.
- **Drivers de Usuario:** El teclado, el ratón y la entrada de juegos se ejecutan en Ring 3 con privilegios controlados.
- **Servidores:** El **Composer** actúa como servidor gráfico, procesando comandos de dibujo de otros procesos.
- **VFS:** Una capa virtual que unifica Initrd (lectura), RamFS (temporal) y CarleyFS (persistente).

## 🚀 Guía de Inicio Rápido

### Requisitos
- GNU Make, GCC (x86_64), xorriso y QEMU.

### Compilación y Ejecución
1. **Configurar el cargador de arranque:**
   ```bash
   make setup
   ```
2. **Compilar el sistema y generar la ISO:**
   ```bash
   make iso
   ```
3. **Lanzar en QEMU:**
   ```bash
   qemu-system-x86_64 -cdrom carley-os.iso -m 512M -smp 4
   ```

## 🗺️ Hoja de Ruta (Roadmap)

- [x] **Núcleo:** SMP, Paging 4-niveles, IPC, Syscalls.
- [x] **Gráficos:** Alpha Blending, Ventanas, Fuentes.
- [x] **FileSystem:** CarleyFS, VFS, IDE Driver.
- [ ] **Redes (0%):** Implementación de stack TCP/IP y driver RTL8139.
- [ ] **USB (0%):** Soporte inicial para controladores UHCI/EHCI.
- [ ] **Estabilidad:** Refactorización de bloqueos de spinlock para evitar deadlocks en alta carga.

## 🤝 Contribuir
Carley OS es un proyecto abierto a la comunidad. Si te apasiona el desarrollo de kernels o el diseño de interfaces de bajo nivel, ¡eres bienvenido!

---
*Desarrollado con ❤️ para el futuro de la computación personal.*
