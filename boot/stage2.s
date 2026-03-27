.code16
.global _start
_start:
    cli
    lgdt gdt_ptr
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0
    ljmp $0x08, $pm_start

.code32
pm_start:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x90000, %esp

    # Preparar el Modo Largo (64 bits)
    # 1. Paginacion basica (identica para el primer GB)
    #    PML4 -> PDPT -> PD (Gigabyte)
    #    Usaremos 0x1000, 0x2000, 0x3000 como tablas fisicas
    mov $0x1000, %edi
    mov %edi, %cr3
    xor %eax, %eax
    mov $4096, %ecx
    rep stosl           # Limpiar 3 paginas

    # PML4[0] = PDPT | PRESENT | WRITABLE
    movl $0x2003, (0x1000)
    # PDPT[0] = PD | PRESENT | WRITABLE
    movl $0x3003, (0x2000)
    # PD[0] = 2MB Page | PRESENT | WRITABLE | PS (0x80)
    movl $0x00000083, (0x3000)

    # 2. Habilitar PAE
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    # 3. Habilitar Long Mode en EFER
    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    # 4. Habilitar Paginacion
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    lgdt gdt_ptr_long
    ljmp $0x08, $long_mode_start

.code64
long_mode_start:
    # Ahora estamos en 64 bits reales!
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %ss

    # El kernel estara en 0x100000 (1MB)
    # Pero antes, para que sea un cargador REAL, cargariamos el ELF.
    # Por ahora saltaremos a una direccion fija para pruebas.
    jmp 0x100000

# GDT para modo protegido 32
gdt_start:
    .quad 0x0000000000000000 # Null
    .quad 0x00CF9A000000FFFF # Code 32
    .quad 0x00CF92000000FFFF # Data 32
gdt_end:

gdt_ptr:
    .short gdt_end - gdt_start - 1
    .long gdt_start

# GDT para modo largo 64
gdt_start_long:
    .quad 0x0000000000000000 # Null
    .quad 0x00209A0000000000 # Code 64 (L bit set)
    .quad 0x0000920000000000 # Data 64
gdt_end_long:

gdt_ptr_long:
    .short gdt_end_long - gdt_start_long - 1
    .long gdt_start_long
