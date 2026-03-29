# Configuración del Compilador
CC ?= gcc
LD ?= ld

KERNEL := kernel.elf

# Eliminamos -mcmodel=kernel porque ahora el cargador es mas simple y
# el kernel se carga en el primer 1MB (Identity mapped)
CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -Isrc/kernel -Icommon

LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld --oformat binary

C_SOURCES := src/kernel/main.c $(filter-out src/kernel/main.c, $(shell find src/kernel -name '*.c')) $(shell find common -name '*.c')
S_SOURCES := $(shell find src/kernel -name '*.s')
OBJ := $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o)

.PHONY: all clean iso userland setup bootloader

all: bootloader $(KERNEL) userland carley-os.img

bootloader:
	nasm -f bin src/boot/boot.asm -o src/boot/boot.bin
	nasm -f bin src/boot/stage2.asm -o src/boot/stage2.bin

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

meta/pack_initrd: meta/pack_initrd.c
	gcc $< -o $@

initrd.bin: meta/pack_initrd userland
	./meta/pack_initrd $@ src/user/*.elf src/user/libc.so

carley-os.img: bootloader $(KERNEL) initrd.bin
	# Crear una imagen de disco de 40MB llena de ceros
	dd if=/dev/zero of=$@ bs=1M count=40
	# Stage 1 (MBR) en sector 1
	dd if=src/boot/boot.bin of=$@ conv=notrunc
	# Stage 2 en sector 2
	dd if=src/boot/stage2.bin of=$@ seek=1 conv=notrunc
	# Kernel en 1MB (Sector 2048)
	dd if=$(KERNEL) of=$@ seek=2048 conv=notrunc
	# Initrd en 10MB (Sector 20480)
	dd if=initrd.bin of=$@ seek=20480 conv=notrunc

userland:
	$(MAKE) -C src/user

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

# Target para generar una ISO booteable (BIOS/Legacy)
iso: carley-os.img
	@echo "Generando carley-os.iso..."
	mkdir -p iso_root
	cp carley-os.img iso_root/
	# Creamos la ISO usando la imagen de disco como sector de arranque El Torito
	xorriso -as mkisofs -R -b carley-os.img -no-emul-boot -boot-load-size 4 -o carley-os.iso iso_root 2>/dev/null || \
	(echo "Error: xorriso no encontrado. Instálalo con: sudo apt install xorriso" && rm -rf iso_root && exit 1)
	rm -rf iso_root
	@echo "¡ISO generada con éxito: carley-os.iso!"

setup:
	@echo "Entorno listo. Asegúrate de tener 'nasm' instalado."
	@echo "Ejecuta: 'sudo apt install nasm xorriso mtools qemu-system-x86'"

clean:
	rm -rf $(OBJ) $(KERNEL) src/boot/*.bin carley-kernel.iso carley-os.iso carley-os.img initrd.bin meta/pack_initrd
	$(MAKE) -C src/user clean
