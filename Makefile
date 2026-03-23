# Configuración del Compilador (Preferiblemente x86_64-elf-gcc)
CC := gcc
LD := ld

# Nombre del binario del kernel
KERNEL := kernel.elf

# Flags del compilador para un kernel de 64 bits sin dependencias externas
CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -I.

# Flags del enlazador (Linker)
LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld

# Archivos fuente y objetos
C_SOURCES := $(shell find kernel -name '*.c')
OBJ := $(C_SOURCES:.c=.o)

.PHONY: all clean iso

all: $(KERNEL)

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Generación de la imagen ISO (Requiere xorriso y Limine instalado/disponible)
iso: $(KERNEL)
	mkdir -p iso_root
	cp $(KERNEL) limine.conf iso_root/
	# Nota: Para una ISO real, necesitamos copiar el binario de limine (limine-bios.sys, etc.)
	# y ejecutar limine-install en el archivo ISO. Este paso asume que el usuario tiene
	# las utilidades de Limine instaladas en su sistema.
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o carley-kernel.iso

clean:
	rm -rf $(OBJ) $(KERNEL) carley-kernel.iso iso_root
