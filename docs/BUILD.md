# Carley Kernel Build System

Este kernel se compila utilizando un compilador cruzado para x86_64 y el cargador de arranque Limine.

## Requisitos del Sistema

Para compilar Carley OS, necesitas un entorno de desarrollo compatible con x86_64 y las siguientes herramientas:

### 1. Herramientas Básicas
- **GNU Make**: Para gestionar la compilación.
- **GCC / G++**: El compilador principal.
- **NASM** (opcional para algunos módulos): Ensamblador.
- **xorriso / mkisofs**: Necesarios para crear la imagen ISO final.

### 2. Instalación de Dependencias

#### En Ubuntu / Debian / WSL:
```bash
sudo apt update
sudo apt install build-essential gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu xorriso mtools
```

#### En macOS:
```bash
brew install x86_64-elf-gcc x86_64-elf-binutils xorriso
```

### 3. El Bootloader (Limine)
Carley OS utiliza el protocolo **Limine**. El Makefile espera que los archivos de Limine estén disponibles para crear la ISO. Puedes clonarlo y compilarlo desde su repositorio oficial:
```bash
git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1
make -C limine
```

## Guía de Compilación Rapida

1. **Compilar todo (Kernel + Userland):**
   ```bash
   make
   ```

2. **Generar la imagen ISO:**
   ```bash
   make iso
   ```
   Esto generará un archivo `carley-os.iso` listo para ser usado en QEMU o VirtualBox.

3. **Ejecutar en QEMU (Recomendado):**
   ```bash
   qemu-system-x86_64 -cdrom carley-os.iso -m 512M -smp 4
   ```

## Solución de Problemas
Si recibes errores sobre "x86_64-elf-gcc not found", asegúrate de que tu compilador cruzado esté en el PATH o edita el `Makefile` para usar el nombre correcto de tu compilador local (ej. `gcc`).
