# Configuración del Compilador
CC := gcc
LD := ld

# Nombre del binario del kernel
KERNEL := kernel.elf

# Flags del compilador
CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -mcmodel=kernel -I.

# Flags del enlazador
LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld

# Archivos fuente y objetos
C_SOURCES := $(shell find kernel -name '*.c') $(shell find common -name '*.c') $(shell find drivers -name '*.c')
S_SOURCES := $(shell find kernel -name '*.s')
OBJ := $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o)

.PHONY: all clean iso

all: $(KERNEL)

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

# Generación de la imagen ISO (Requiere xorriso y binarios de Limine)
iso: $(KERNEL)
	mkdir -p iso_root/boot
	cp $(KERNEL) iso_root/boot/
	cp limine.conf iso_root/boot/
	# Nota: Se asume que el usuario tiene xorriso instalado.
	# Los binarios de limine deben estar en el path para un arranque real.
	xorriso -as mkisofs -b boot/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o carley-kernel.iso || echo "Xorriso fallo: Asegurese de tener las herramientas de Limine."

clean:
	rm -rf $(OBJ) $(KERNEL) carley-kernel.iso iso_root
