[bits 16]
[org 0x8000]

stage2_start:
    cli
    ; 1. Pasar de 16-bit a 32-bit (Modo Protegido)
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_start

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; 2. Preparar el Paso a Modo Largo (64-bit)
    ;    Crearemos la tabla de páginas inicial en 0x1000
    ;    PML4 -> PDPT -> PD -> 2MB Pages
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd

    ; PML4[0] = PDPT (0x2000) | PRESENT | WRITABLE
    mov dword [0x1000], 0x2003
    ; PDPT[0] = PD (0x3000) | PRESENT | WRITABLE
    mov dword [0x2000], 0x3003
    ; PD[0...511] = 512 páginas de 2MB cada una (Mapeamos los primeros 1GB)
    mov edi, 0x3000
    mov eax, 0x83        ; PageSize(0x80) | Writable(0x2) | Present(0x1)
    mov ecx, 512
map_loop:
    mov [edi], eax
    add eax, 0x200000    ; Siguiente página de 2MB
    add edi, 8
    loop map_loop

    ; Habilitar PAE (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Habilitar Long Mode en EFER MSR
    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Habilitar Paginación
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt_ptr_long]
    jmp 0x08:long_mode_start

[bits 64]
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; Saltar al kernel. El kernel estará cargado en la dirección fija 0x100000 (1MB)
    ; (El Makefile pegará el kernel después del Stage 2)
    mov rax, 0x100000
    jmp rax

; GDT para Modo Protegido (32-bit)
gdt_start:
    dq 0x0000000000000000
    dq 0x00cf9a000000ffff ; Código
    dq 0x00cf92000000ffff ; Datos
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; GDT para Modo Largo (64-bit)
gdt_start_long:
    dq 0x0000000000000000
    dq 0x00209a0000000000 ; Código 64
    dq 0x0000920000000000 ; Datos 64
gdt_end_long:

gdt_ptr_long:
    dw gdt_end_long - gdt_start_long - 1
    dq gdt_start_long
