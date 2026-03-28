# Configuración del Compilador
CC ?= gcc
LD ?= ld

KERNEL := kernel.elf

# Eliminamos -mcmodel=kernel porque ahora el cargador es mas simple y
# el kernel se carga en el primer 1MB (Identity mapped)
CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -I.

LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld --oformat binary

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

tools/pack_initrd: tools/pack_initrd.c
	gcc $< -o $@

initrd.bin: tools/pack_initrd userland
	./tools/pack_initrd $@ user/*.elf user/libc.so

carley-os.img: bootloader $(KERNEL) initrd.bin
	# Crear una imagen de disco de 40MB llena de ceros
	dd if=/dev/zero of=$@ bs=1M count=40
	# Stage 1 (MBR) en sector 1
	dd if=boot/boot.bin of=$@ conv=notrunc
	# Stage 2 en sector 2
	dd if=boot/stage2.bin of=$@ seek=1 conv=notrunc
	# Kernel en 1MB (Sector 2048)
	dd if=$(KERNEL) of=$@ seek=2048 conv=notrunc
	# Initrd en 10MB (Sector 20480)
	dd if=initrd.bin of=$@ seek=20480 conv=notrunc

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
