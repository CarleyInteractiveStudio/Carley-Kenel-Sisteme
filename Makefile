# Configuración del Compilador
CC := gcc
LD := ld

KERNEL := kernel.elf

CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -mcmodel=kernel -I.

LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld

C_SOURCES := $(shell find kernel -name '*.c') $(shell find common -name '*.c') $(shell find drivers -name '*.c')
S_SOURCES := $(shell find kernel -name '*.s')
OBJ := $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o)

.PHONY: all clean iso userland setup

all: $(KERNEL) userland

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

userland:
	$(MAKE) -C user

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

iso: $(KERNEL) userland
	mkdir -p iso_root/boot
	cp $(KERNEL) iso_root/boot/
	cp limine.conf iso_root/boot/
	cp user/*.elf iso_root/boot/
	cp user/*.so iso_root/boot/
	# Tambien copiar archivos de script/datos si existen
	@if [ -f "user/demo.py" ]; then cp user/demo.py iso_root/boot/; fi

	# Copiar binarios de Limine (asumiendo que están en ./limine/)
	@if [ -d "limine" ]; then \
		cp limine/limine-bios.sys iso_root/boot/ ; \
		cp limine/limine-bios-cd.bin iso_root/boot/ ; \
		cp limine/limine-uefi-cd.bin iso_root/boot/ ; \
	fi

	# Verificar si xorriso está instalado para generar la ISO real
	@if command -v xorriso > /dev/null; then \
		if [ -f "iso_root/boot/limine-bios-cd.bin" ]; then \
			xorriso -as mkisofs -b boot/limine-bios-cd.bin \
				-no-emul-boot -boot-load-size 4 -boot-info-table \
				--efi-boot boot/limine-uefi-cd.bin \
				-efi-boot-part --efi-boot-image --protective-msdos-label \
				iso_root -o carley-os.iso && \
			echo "¡ÉXITO! carley-os.iso generado para VirtualBox."; \
		else \
			echo "ERROR: No se encuentran los archivos de Limine en ./limine/. Ejecuta: git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1"; \
			exit 1; \
		fi \
	else \
		echo "ERROR: xorriso no detectado. Instálalo con 'sudo apt install xorriso'."; \
		exit 1; \
	fi

	dd if=/dev/zero of=carley-disk.img bs=1M count=10

setup:
	@echo "Configurando entorno de arranque..."
	rm -rf limine
	git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1
	$(MAKE) -C limine
	@echo "Entorno listo. Ya puedes ejecutar 'make iso'."

clean:
	rm -rf $(OBJ) $(KERNEL) carley-kernel.iso carley-os.iso iso_root carley-disk.img
	$(MAKE) -C user clean
