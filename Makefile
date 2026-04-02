# Configuración del Compilador
CC ?= gcc
LD ?= ld

KERNEL := kernel.elf

# CFLAGS para Kernel de 64 bits con Limine
CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -mcmodel=kernel -fcf-protection=none -Isrc/kernel -Icommon

# LDFLAGS para generar un ELF64 real
LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld

C_SOURCES := $(shell find src/kernel -name '*.c') $(shell find common -name '*.c')
S_SOURCES := $(shell find src/kernel -name '*.s')
OBJ := $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o)

.PHONY: all clean iso userland setup

all: $(KERNEL) userland initrd.bin

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

meta/pack_initrd: meta/pack_initrd.c
	gcc $< -o $@

initrd.bin: meta/pack_initrd userland
	./meta/pack_initrd $@ src/user/*.elf src/user/libc.so

userland:
	$(MAKE) -C src/user

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

# Generar ISO usando Limine - Corrigiendo rutas de búsqueda del config
iso: $(KERNEL) initrd.bin
	@echo "Generando carley-os.iso con Limine (Fixing Config Path)..."
	rm -rf iso_root
	mkdir -p iso_root/boot/limine

	# El Kernel y el Initrd se quedan en /boot/
	cp $(KERNEL) iso_root/boot/
	cp initrd.bin iso_root/boot/

	# El archivo de configuración y el sistema de Limine en la raíz Y en /boot/limine/ para máxima compatibilidad
	cp limine.conf iso_root/
	cp limine.conf iso_root/boot/limine/

	cp limine/limine-bios.sys iso_root/boot/limine/
	cp limine/limine-bios-cd.bin iso_root/boot/limine/
	cp limine/limine-uefi-cd.bin iso_root/boot/limine/

	xorriso -as mkisofs \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o carley-os.iso

	./limine/limine bios-install carley-os.iso
	rm -rf iso_root
	@echo "¡ISO generada con éxito! He puesto limine.conf en la raíz para evitar el error 'Config file not found'."

setup:
	@echo "Instalando dependencias de Limine..."
	if [ ! -f "limine/limine" ]; then \
		rm -rf limine; \
		git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1; \
		$(MAKE) -C limine; \
	fi
	@echo "Dependencias de Limine instaladas correctamente."

clean:
	rm -rf $(OBJ) $(KERNEL) carley-os.iso carley-os.img initrd.bin meta/pack_initrd
	$(MAKE) -C src/user clean
