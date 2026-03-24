# Configuración del Compilador
CC := gcc
LD := ld

KERNEL := kernel.elf

CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-stack-check \
          -fno-lto -fno-pie -fno-pic -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse \
          -mno-sse2 -mno-red-zone -mcmodel=kernel -I.

LDFLAGS := -nostdlib -static -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld

C_SOURCES := $(shell find kernel -name '*.c') $(shell find common -name '*.c') $(shell find drivers -name '*.c')
S_SOURCES := $(shell find kernel -name '*.s')
OBJ := $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o)

.PHONY: all clean iso userland

all: $(KERNEL) userland

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

userland:
	$(MAKE) -C user

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

iso: $(KERNEL) userland
	mkdir -p iso_root/boot
	cp $(KERNEL) iso_root/boot/
	cp limine.conf iso_root/boot/
	cp user/shell.elf iso_root/shell.elf
	cp user/hello.elf iso_root/hello.elf
	cp user/write.elf iso_root/write.elf
	cp user/edit.elf iso_root/edit.elf
	cp user/gui.elf iso_root/gui.elf
	cp user/casm.elf iso_root/casm.elf
	cp user/crun.elf iso_root/crun.elf
	dd if=/dev/zero of=carley-disk.img bs=1M count=10
	# Nota técnica: Para generar una ISO arrancable real se requiere xorriso:
	# xorriso -as mkisofs -b boot/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
	# --efi-boot boot/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label \
	# iso_root -o carley-kernel.iso
	@echo "Estructura ISO y Disco HD listos en iso_root/ y carley-disk.img"

clean:
	rm -rf $(OBJ) $(KERNEL) carley-kernel.iso iso_root carley-disk.img
	$(MAKE) -C user clean
