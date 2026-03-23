# Configuración para el Espacio de Usuario
CC := gcc
AR := ar
AS := gcc

CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fno-pie -fno-pic -m64 -Ilibc/include
LDFLAGS := -nostdlib -static

LIBC_OBJ := libc/string.o libc/stdio.o libc/stdlib.o libc/arch/x86_64/crt0.o libc/arch/x86_64/syscall.o
LIBC := libc.a

.PHONY: all clean

all: $(LIBC) hello.elf shell.elf write.elf

$(LIBC): $(LIBC_OBJ)
	$(AR) rcs $@ $(LIBC_OBJ)

hello.elf: hello.o $(LIBC)
	$(CC) $(LDFLAGS) -T linker.ld hello.o $(LIBC) -o $@

shell.elf: shell.o $(LIBC)
	$(CC) $(LDFLAGS) -T linker.ld shell.o $(LIBC) -o $@

write.elf: write.o $(LIBC)
	$(CC) $(LDFLAGS) -T linker.ld write.o $(LIBC) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBC_OBJ) $(LIBC) hello.o hello.elf shell.o shell.elf write.o write.elf
