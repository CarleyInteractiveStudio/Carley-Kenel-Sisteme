# Carley Kernel Build System

Este kernel se compila utilizando un compilador cruzado para x86_64 y el cargador de arranque Limine.

## Requisitos
- GCC (x86_64-elf) o clang
- GNU Make
- Limine (última versión estable)
- xorriso (para generar la imagen ISO)

## Comandos Disponibles
- `make`: Compila el kernel.
- `make iso`: Genera la imagen ISO arrancable.
- `make clean`: Limpia los archivos generados.
