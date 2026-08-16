# Makefile Principal para Carley Operating System (COS) y CK Kernel
# Todo compilado usando el lenguaje K y el compilador kcc (project-K)

KCC = ./project-K/kcc
PYTHON = python3

.PHONY: all compiler bootloader kernel os iso clean

all: compiler bootloader kernel os iso

compiler:
	@echo "==> Compilando el compilador kcc..."
	$(MAKE) -C project-K

bootloader: compiler
	@echo "==> Compilando el Arrancador (boot.k)..."
	$(KCC) arrancador/boot.k -o arrancador/boot.bin

kernel: compiler
	@echo "==> Compilando el Kernel CK (kernel/main.k)..."
	$(KCC) kernel/main.k -o kernel/kernel.bin

os: compiler
	@echo "==> Compilando Carley Operating System (sistema_operativo/main.k)..."
	$(KCC) sistema_operativo/main.k -o sistema_operativo/cos.bin

iso: os
	@echo "==> Generando la imagen de disco (.img) y la ISO booteable (.iso)..."
	$(PYTHON) meta/make_iso.py sistema_operativo/cos.bin carley-os.img carley-os.iso

clean:
	@echo "==> Limpiando binarios..."
	rm -f arrancador/*.bin arrancador/*.bin.s
	rm -f kernel/*.bin kernel/*.bin.s
	rm -f sistema_operativo/*.bin sistema_operativo/*.bin.s
	rm -f carley-os.iso carley-os.img kernel.bin kernel.bin.s meta/*.o meta/*.bin
	$(MAKE) -C project-K clean
