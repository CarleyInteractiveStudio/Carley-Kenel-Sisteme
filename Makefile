# Configuración del Compilador
CC ?= gcc
LD ?= ld

KERNEL := kernel.elf

# CFLAGS para Kernel de 64 bits con Limine
# Importante: -mcmodel=kernel para el direccionamiento ffffffff80000000
CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -mcmodel=kernel -fcf-protection=none -Isrc/kernel -Icommon

# LDFLAGS para generar un ELF64 real (no binario crudo)
LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld

C_SOURCES := $(shell find src/kernel -name '*.c') $(shell find common -name '*.c')
S_SOURCES := $(shell find src/kernel -name '*.s')
# Excluimos entry.o ya que ahora el kernel empieza en main.c (_start)
OBJ := $(filter-out src/kernel/entry.o, $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o))

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

# Generar ISO usando Limine
iso: $(KERNEL) initrd.bin
	@echo "Generando carley-os.iso con Limine..."
	rm -rf iso_root
	mkdir -p iso_root/boot
	cp $(KERNEL) iso_root/boot/
	cp initrd.bin iso_root/boot/
	cp limine.conf iso_root/boot/
	cp limine/limine-bios.sys iso_root/boot/
	cp limine/limine-bios-cd.bin iso_root/boot/
	cp limine/limine-uefi-cd.bin iso_root/boot/
	xorriso -as mkisofs \
		-b boot/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o carley-os.iso
	./limine/limine bios-install carley-os.iso
	rm -rf iso_root
	@echo "¡ISO generada con éxito con Limine!"

setup:
	@echo "Instalando dependencias de Limine..."
	git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1
	make -C limine
	@echo "Dependencias de Limine instaladas."

clean:
	rm -rf $(OBJ) $(KERNEL) carley-os.iso carley-os.img initrd.bin meta/pack_initrd
	$(MAKE) -C src/user clean
