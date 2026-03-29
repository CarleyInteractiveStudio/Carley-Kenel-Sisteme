[bits 16]
[org 0x1000]

ap_trampoline:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; 1. GDT Temporal
    lgdt [gdt_ptr_ap - ap_trampoline + 0x1000]

    ; 2. Habilitar Modo Protegido
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:(pm_start_ap - ap_trampoline + 0x1000)

[bits 32]
pm_start_ap:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; 3. Paginación (Usamos las tablas del kernel ya en 0x20000)
    mov eax, 0x20000
    mov cr3, eax

    ; 4. Long Mode
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt_ptr_long_ap - ap_trampoline + 0x1000]
    jmp 0x08:(long_mode_start_ap - ap_trampoline + 0x1000)

[bits 64]
long_mode_start_ap:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; El stack y entry point del AP se pasan via direcciones fijas por el Kernel
    mov rsp, [0x1500]
    mov rax, [0x1508]
    call rax
    jmp $

align 16
gdt_start_ap:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end_ap:
gdt_ptr_ap:
    dw gdt_end_ap - gdt_start_ap - 1
    dd gdt_start_ap - ap_trampoline + 0x1000

gdt_start_long_ap:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end_long_ap:
gdt_ptr_long_ap:
    dw gdt_end_long_ap - gdt_start_long_ap - 1
    dq gdt_start_long_ap - ap_trampoline + 0x1000

ap_trampoline_end:
