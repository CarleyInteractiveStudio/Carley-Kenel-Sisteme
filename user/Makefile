# Configuración para el Espacio de Usuario
CC := gcc
CXX := g++
AR := ar
AS := gcc

CFLAGS := -Wall -Wextra -std=c11 -ffreestanding -fno-stack-protector -fPIC -m64 -Ilibc/include -I..
CXXFLAGS := -Wall -Wextra -std=c++11 -ffreestanding -fno-stack-protector -fno-exceptions -fno-rtti -fPIC -m64 -Ilibc/include -I..
LDFLAGS_STATIC := -nostdlib -static
LDFLAGS_DYNAMIC := -nostdlib -shared

LIBC_OBJ := libc/string.o libc/stdio.o libc/stdlib.o libc/time.o libc/math.o libc/arch/x86_64/crt0.o libc/arch/x86_64/syscall.o libc/cpp_runtime.o libc/bmp.o
LIBC_A := libc.a
LIBC_SO := libc.so
LD_SO := ldso.elf

PROGS_STATIC := hello.elf shell.elf write.elf edit.elf gui.elf casm.elf crun.elf calc.elf play.elf game.elf clock.elf input.elf cpython.elf explorer.elf setup.elf code.elf shm_demo.elf about.elf term.elf settings.elf
PROGS_DYNAMIC := hello_dyn.elf dashboard.elf

.PHONY: all clean

all: $(LIBC_A) $(LIBC_SO) $(LD_SO) $(PROGS_STATIC) $(PROGS_DYNAMIC)

$(LIBC_A): $(LIBC_OBJ)
	$(AR) rcs $@ $(LIBC_OBJ)

$(LIBC_SO): $(LIBC_OBJ)
	$(CC) $(LDFLAGS_DYNAMIC) -T linker.ld $(LIBC_OBJ) -o $@

$(LD_SO): ldso.o ldso_crt0.o $(LIBC_A)
	$(CC) $(LDFLAGS_STATIC) -T ldso.ld ldso_crt0.o ldso.o $(LIBC_A) -o $@

%.elf: %.o $(LIBC_A)
	$(CC) $(LDFLAGS_STATIC) -T linker.ld $< $(LIBC_A) -o $@

hello_dyn.elf: hello.o $(LIBC_SO)
	$(CC) -nostdlib -T linker.ld hello.o $(LIBC_SO) -Wl,--dynamic-linker=/ldso.elf -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBC_OBJ) $(LIBC) *.o *.elf
