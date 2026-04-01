# Configuración del Compilador
CC ?= gcc
LD ?= ld

KERNEL := kernel.elf

# Eliminamos -mcmodel=kernel porque ahora el cargador es mas simple y
# el kernel se carga en el primer 1MB (Identity mapped)
CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -mcmodel=large -fcf-protection=none -Isrc/kernel -Icommon

LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld --oformat binary

C_SOURCES := src/kernel/main.c $(filter-out src/kernel/main.c, $(shell find src/kernel -name '*.c')) $(shell find common -name '*.c')
S_SOURCES := $(shell find src/kernel -name '*.s')
# Explicitly place main.o first to ensure the magic signature is at the start of the binary
OBJ := src/kernel/entry.o $(filter-out src/kernel/entry.o, $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o))

.PHONY: all clean iso userland setup bootloader

all: bootloader $(KERNEL) userland carley-os.img

# UEFI Loader CFLAGS
UEFI_CFLAGS := -ffreestanding -fshort-wchar -mno-red-zone -fno-stack-protector \
               -target x86_64-unknown-windows -c

bootloader:
	nasm -f bin src/boot/boot.asm -o src/boot/boot.bin
	nasm -f bin src/boot/stage2.asm -o src/boot/stage2.bin
	nasm -f bin src/boot/ap_trampoline.asm -o src/boot/ap_trampoline.bin
	cat src/boot/boot.bin src/boot/stage2.bin > bootloader.bin
	# Intentar compilar el cargador UEFI si gcc-mingw-w64 está disponible
	-x86_64-w64-mingw32-gcc $(UEFI_CFLAGS) src/boot/uefi/main.c -o src/boot/uefi/main.o
	-x86_64-w64-mingw32-gcc -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 src/boot/uefi/main.o -o bootx64.efi

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

meta/pack_initrd: meta/pack_initrd.c
	gcc $< -o $@

meta/format_carleyfs: meta/format_carleyfs.cpp
	g++ $< -o $@

initrd.bin: meta/pack_initrd userland
	./meta/pack_initrd $@ src/user/*.elf src/user/libc.so

carley-os.img: bootloader $(KERNEL) initrd.bin meta/format_carleyfs
	# Generar imagen de disco con CarleyFS (Superbloque, Inodos y archivos)
	./meta/format_carleyfs $@ bootloader.bin $(KERNEL) initrd.bin src/boot/ap_trampoline.bin

userland:
	$(MAKE) -C src/user

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

# Target para generar una ISO booteable (BIOS/Legacy)
iso: carley-os.img bootloader
	@echo "Generando carley-os.iso..."
	mkdir -p iso_root
	cp carley-os.img iso_root/
	xorriso -as mkisofs \
		-quiet \
		-V "CARLEY_OS" \
		-b carley-os.img \
		-no-emul-boot \
		-boot-load-size 64 \
		-o carley-os.iso iso_root || \
	(echo "Error: xorriso falló al crear ISO." && rm -rf iso_root && exit 1)
	rm -rf iso_root
	@echo "¡ISO generada con éxito: carley-os.iso!"

setup:
	@echo "Entorno listo. Asegúrate de tener 'nasm' instalado."
	@echo "Ejecuta: 'sudo apt install nasm xorriso mtools qemu-system-x86'"

clean:
	rm -rf $(OBJ) $(KERNEL) src/boot/*.bin bootloader.bin carley-kernel.iso carley-os.iso carley-os.img initrd.bin meta/pack_initrd meta/format_carleyfs src/boot/ap_trampoline.bin bootx64.efi src/boot/uefi/*.o
	$(MAKE) -C src/user clean
