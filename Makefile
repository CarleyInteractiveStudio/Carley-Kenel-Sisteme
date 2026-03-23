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

# Generación de la imagen ISO
iso: $(KERNEL)
	mkdir -p iso_root/boot
	cp $(KERNEL) iso_root/boot/
	cp limine.conf iso_root/boot/
	# Crear un initrd de prueba
	echo -n -e "\x01\x00\x00\x00welcome.txt\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x15\x00\x00\x00\x48\x00\x00\x00Hola desde el Initrd!" > iso_root/boot/initrd.bin
	# xorriso ... (mismas instrucciones previas)
	@echo "ISO lista para empaquetar con xorriso."

clean:
	rm -rf $(OBJ) $(KERNEL) carley-kernel.iso iso_root
