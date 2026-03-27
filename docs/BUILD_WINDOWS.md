# 🪟 Guía de Compilación en Windows (WSL2)

Para compilar y probar **Carley OS** en Windows, la forma más rápida y estable es utilizar **WSL2 (Windows Subsystem for Linux)**. Esto te permite usar herramientas de compilación de Linux directamente en Windows sin necesidad de una máquina virtual pesada.

## 1. Configuración de WSL2

1.  Abre **PowerShell** como administrador y ejecuta:
    ```powershell
    wsl --install
    ```
    *Si ya tienes WSL instalado, asegúrate de que sea la versión 2 (`wsl --set-default-version 2`).*
2.  Reinicia tu computadora si es necesario.
3.  Instala **Ubuntu** desde la Microsoft Store si no se instaló automáticamente.
4.  Abre la terminal de Ubuntu y configura tu usuario y contraseña.

## 2. Instalación de Herramientas de Desarrollo

Dentro de la terminal de Ubuntu, ejecuta los siguientes comandos para instalar el compilador y las utilidades necesarias:

```bash
sudo apt update
sudo apt install build-essential git gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu xorriso mtools qemu-system-x86
```

## 3. Descarga y Compilación de Carley OS

1.  **Clona tu repositorio** (o entra a la carpeta del proyecto):
    ```bash
    git clone <tu-repositorio-url>
    cd carley-os
    ```

2.  **Configura el cargador de arranque (Limine):**
    Este paso solo se hace una vez para preparar los archivos de arranque:
    ```bash
    make setup
    ```

3.  **Compila el Kernel y las Apps:**
    ```bash
    make
    ```

4.  **Genera la imagen ISO:**
    ```bash
    make iso
    ```
    *Esto creará el archivo `carley-os.iso` en la carpeta raíz.*

## 4. Cómo Probarlo en Windows

### Opción A: Usar QEMU dentro de WSL (Recomendado)
Si instalaste `qemu-system-x86` en el paso anterior, puedes ejecutar:
```bash
qemu-system-x86_64 -cdrom carley-os.iso -m 512M -smp 4
```
*Si tienes instalado un servidor X o WSLg (Windows 11), verás la ventana de Carley OS aparecer directamente.*

### Opción B: Usar VirtualBox o VMware en Windows
1.  Busca el archivo `carley-os.iso` usando el Explorador de Archivos de Windows (puedes acceder a tus archivos de WSL escribiendo `\\wsl$` en la barra de direcciones).
2.  Crea una nueva máquina virtual en VirtualBox:
    *   **Tipo:** Other / Other (64-bit).
    *   **Memoria:** 1024 MB.
    *   **Procesadores:** 2 o 4 (para probar SMP).
    *   **Almacenamiento:** Monta el archivo `carley-os.iso` como unidad de CD.

---
**¿Tienes problemas?** Si recibes errores de "command not found", asegúrate de que todos los paquetes en el Paso 2 se instalaron correctamente.
