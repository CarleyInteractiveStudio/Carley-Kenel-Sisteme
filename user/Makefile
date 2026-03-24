# Configuración para el Espacio de Usuario
CC := gcc
AR := ar
AS := gcc

CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-pie -fno-pic -m64 -Ilibc/include -I..
LDFLAGS := -nostdlib -static

LIBC_OBJ := libc/string.o libc/stdio.o libc/stdlib.o libc/arch/x86_64/crt0.o libc/arch/x86_64/syscall.o
LIBC := libc.a

PROGS := hello.elf shell.elf write.elf edit.elf gui.elf casm.elf crun.elf

.PHONY: all clean

all: $(LIBC) $(PROGS)

$(LIBC): $(LIBC_OBJ)
	$(AR) rcs $@ $(LIBC_OBJ)

%.elf: %.o $(LIBC)
	$(CC) $(LDFLAGS) -T linker.ld $< $(LIBC) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBC_OBJ) $(LIBC) *.o *.elf
