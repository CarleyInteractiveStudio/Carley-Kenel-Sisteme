# 🐧 Guía de Compilación en Ubuntu (Nativo)

Esta es la guía principal para compilar **Carley OS** en un entorno Linux Ubuntu nativo. Se recomienda Ubuntu 22.04 LTS o superior.

## 1. Instalación de Dependencias

Ejecuta el siguiente comando para instalar todas las herramientas necesarias para la compilación del kernel, el entorno de usuario y la generación de la ISO:

```bash
sudo apt update
sudo apt install build-essential git gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu xorriso mtools qemu-system-x86
```

## 2. Preparación del Cargador de Arranque (Limine)

Carley OS utiliza el protocolo Limine. Debes preparar los archivos de arranque antes de compilar la ISO. Este paso solo es necesario una vez:

```bash
make setup
```

## 3. Compilación del Sistema

Para compilar el microkernel y todas las aplicaciones de usuario (`userland`), simplemente ejecuta:

```bash
make
```

## 4. Generación de la Imagen ISO

Una vez que el kernel (`kernel.elf`) y los ejecutables de usuario están listos, genera la imagen ISO arrancable:

```bash
make iso
```
Esto producirá el archivo `carley-os.iso` en el directorio raíz del proyecto.

## 5. Ejecución y Pruebas

### En QEMU (Emulador)
La forma más rápida de probar tus cambios es usando QEMU:
```bash
qemu-system-x86_64 -cdrom carley-os.iso -m 512M -smp 4
```

### En Hardware Real o VirtualBox
Puedes copiar `carley-os.iso` a un USB (usando `dd` o BalenaEtcher) o montarlo en VirtualBox como una unidad óptica.
- **Configuración recomendada en VirtualBox:**
  - **Tipo:** Other / Other (64-bit).
  - **Memoria:** 1024 MB.
  - **CPU:** 2 o más núcleos (para activar SMP).

---
**¿Errores comunes?**

### Error de QEMU (Symbol lookup error / GLIBC_PRIVATE)
Si al ejecutar QEMU ves un error como `undefined symbol: __libc_pthread_init, version GLIBC_PRIVATE`, es porque la versión de QEMU instalada mediante **Snap** tiene un conflicto con las librerías de tu sistema Ubuntu.

**Solución:** Desinstala la versión Snap e instala la versión nativa de los repositorios:
```bash
sudo snap remove qemu
sudo apt install qemu-system-x86
```

- Si `make iso` falla con un error de `xorriso`, asegúrate de que el paquete esté instalado.
- Si el kernel no arranca en VirtualBox, verifica que la aceleración VT-x/AMD-V esté activada en la BIOS de tu computadora real (aunque en Linux nativo esto suele ser menos problemático que en WSL).
