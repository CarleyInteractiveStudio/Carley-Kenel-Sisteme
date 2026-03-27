# Configuración del Compilador
# Puedes sobrescribir estas variables para compilación cruzada
CC ?= gcc
LD ?= ld

KERNEL := kernel.elf

CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -mcmodel=kernel -I.

LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld

C_SOURCES := $(shell find kernel -name '*.c') $(shell find common -name '*.c') $(shell find drivers -name '*.c')
S_SOURCES := $(shell find kernel -name '*.s')
OBJ := $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o)

.PHONY: all clean iso userland setup bootloader

all: bootloader $(KERNEL) userland carley-os.img

bootloader:
	nasm -f bin boot/boot.asm -o boot/boot.bin
	nasm -f bin boot/stage2.asm -o boot/stage2.bin

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

carley-os.img: bootloader $(KERNEL)
	# Crear una imagen de disco de 20MB llena de ceros
	dd if=/dev/zero of=$@ bs=1M count=20
	# Escribir el Stage 1 (MBR) en el primer sector (512 bytes)
	dd if=boot/boot.bin of=$@ conv=notrunc
	# Escribir el Stage 2 justo después (Sector 2 en adelante)
	dd if=boot/stage2.bin of=$@ seek=1 conv=notrunc
	# Escribir el Kernel en la posición 1MB (2048 sectores)
	# Esto coincide con el salto 'jmp 0x100000' en stage2.asm
	dd if=$(KERNEL) of=$@ seek=2048 conv=notrunc

userland:
	$(MAKE) -C user

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

# Mantenemos el target iso por compatibilidad, pero ahora recomendamos carley-os.img
iso: carley-os.img
	@echo "AVISO: Se ha generado carley-os.img (imagen de disco duro)."
	@echo "Se recomienda usar carley-os.img como disco duro en VirtualBox."

setup:
	@echo "Configurando entorno de arranque..."
	rm -rf limine
	git clone https://github.com/limine-bootloader/limine.git --branch=v7.x-binary --depth=1
	$(MAKE) -C limine
	@echo "Entorno listo. Ya puedes ejecutar 'make iso'."

clean:
	rm -rf $(OBJ) $(KERNEL) carley-kernel.iso carley-os.iso iso_root carley-disk.img
	$(MAKE) -C user clean
