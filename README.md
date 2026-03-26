# Carley OS

Carley OS es un sistema operativo moderno, seguro y visualmente atractivo diseñado para PCs y consolas. Es un proyecto desarrollado desde cero, independiente de Linux, enfocado en gaming, programación y desarrollo de videojuegos.

## Características Principales

*   **Microkernel x86_64:** Arquitectura basada en el paso de mensajes (IPC) y servicios en espacio de usuario.
*   **Interfaz de Usuario "Apple-Style":** Dashboard con efectos de cristal (Alpha Blending) y lanzador central de aplicaciones.
*   **Seguridad por Capacidades:** Sistema de permisos que restringe el acceso al hardware y al disco por proceso.
*   **Symmetric Multiprocessing (SMP):** Soporte real para múltiples núcleos de CPU.
*   **Entorno de Desarrollo Integrado:**
    *   Soporte nativo para C y C++.
    *   Intérprete de Python (Carley Script) integrado de fábrica.
    *   Hoja de ruta para compiladores de C# y motores JS/HTML/CSS.
*   **Almacenamiento Persistente:** CarleyFS, un sistema de archivos diseñado para la estabilidad y el rendimiento en discos IDE/SATA.
*   **Multimedia:** Sistema de sonido SB16 con mezclador multicanal de 4 vías.

## Aplicaciones de Fábrica

*   **Dashboard:** Lanzador gráfico principal con burbuja de cristal.
*   **Carley Explorer:** Explorador de archivos con iconos reales.
*   **Carley Code:** Editor de código moderno con temas oscuros.
*   **Carley Setup:** Instalador interactivo para configurar idioma y periféricos.
*   **Terminal:** Shell potente para gestión avanzada del sistema.

## Desarrollo

Carley OS utiliza C para el núcleo y C++ para las aplicaciones de alto nivel para garantizar la máxima eficiencia y potencia.

### 🛠️ Guía de Inicio Rápido para Desarrolladores

Si quieres compilar y probar Carley OS tú mismo, sigue estos pasos:

1. **Instala las dependencias:** Consulta la [Guía de Compilación](docs/BUILD.md) para ver los comandos específicos de tu sistema operativo (Linux/macOS).
2. **Compila el proyecto:**
   ```bash
   make
   ```
3. **Crea la ISO:**
   ```bash
   make iso
   ```
4. **Pruébalo en QEMU:**
   ```bash
   qemu-system-x86_64 -cdrom carley-kernel.iso -m 512M
   ```

Para más detalles técnicos, revisa la carpeta `docs/`.
