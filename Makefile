# Configuración del Compilador
CC := gcc
LD := ld

# Nombre del binario del kernel
KERNEL := kernel.elf

# Flags del compilador
CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -I.

# Flags del enlazador
LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld

# Archivos fuente y objetos
C_SOURCES := $(shell find kernel -name '*.c') $(shell find common -name '*.c')
OBJ := $(C_SOURCES:.c=.o)

.PHONY: all clean iso

all: $(KERNEL)

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(KERNEL) carley-kernel.iso iso_root
