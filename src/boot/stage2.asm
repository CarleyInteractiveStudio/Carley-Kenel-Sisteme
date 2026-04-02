[bits 16]
[org 0x7E00]

; FIRMA MÁGICA PARA EL MBR
dd 0xDEADBEEF

stage2_entry:
    ; Marcador 'S' (Stage 2 ALCANZADO)
    mov al, 'S'
    mov ah, 0x0e
    int 0x10

    ; Normalización total
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    ; Trace: Mensaje de Bienvenida
    mov si, msg_welcome
    call print_string

    ; Hardware init: A20
    call enable_a20
    mov al, 'A'
    call print_char

    ; Entrar en Unreal Mode (Cargar FS/GS con límite de 4GB)
    ; Versión ULTRA-reforzada para VirtualBox
    cli
    push ds
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; Salto lejano PM 16-bit
    jmp 0x18:.pm_temp

.pm_temp:
    mov ax, 0x10 ; Selector 4GB
    mov fs, ax
    mov gs, ax

    mov eax, cr0
    and al, 0xFE
    mov cr0, eax

    ; Salto lejano de vuelta a Modo Real (Normalizar CS)
    jmp 0:.real_temp

.real_temp:
    pop ds
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov al, 'U'
    call print_char

    ; --- RESTO DEL CARGADOR (E820, VBE, KERNEL LOAD) ---
    ; Detección de memoria
    mov di, 0x9000
    xor ebx, ebx
    mov dword [mem_count_extended], 0
.do_e820:
    mov eax, 0xe820
    mov ecx, 24
    mov edx, 0x534D4150
    int 0x15
    jc .e820_done
    add di, 24
    inc dword [mem_count_extended]
    test ebx, ebx
    jne .do_e820
.e820_done:
    mov al, 'M'
    call print_char

    ; VBE
    mov ax, 0x4f01
    mov cx, 0x4118
    mov di, 0x7000
    int 0x10
    mov al, 'G'
    call print_char

    ; Cargar Superbloque y Kernel
    ; [Detección robusta de sector_factor]
    mov dword [dap_lba_s2], 128
    call read_one_sector
    jnc .check_sb
    mov dword [dap_lba_s2], 32
    call read_one_sector
.check_sb:
    mov dword [sector_factor], 1
    cmp dword [dap_lba_s2], 32
    jne .read_inodes
    mov dword [sector_factor], 4

.read_inodes:
    ; Cargar tabla de inodos a 0x30000
    mov eax, 132
    xor edx, edx
    div dword [sector_factor]
    mov [dap_lba_s2], eax
    mov edi, 0x30000
    mov ecx, 8
.inode_loop:
    push ecx
    call read_one_sector
    call copy_to_high_mem
    inc dword [dap_lba_s2]
    add edi, 512
    pop ecx
    loop .inode_loop

    ; Cargar Kernel a 0x100000 (Simplificado para el fix)
    mov edi, 0x100000
    mov dword [kernel_lba_current], 256
.load_kernel_loop:
    mov eax, [kernel_lba_current]
    xor edx, edx
    div dword [sector_factor]
    mov [dap_lba_s2], eax
    call read_one_sector
    call copy_to_high_mem
    add edi, 512
    mov eax, [sector_factor]
    add [kernel_lba_current], eax
    cmp edi, 0x180000
    jl .load_kernel_loop

    mov al, 'K'
    call print_char

    ; Configurar modo gráfico final
    mov ax, 0x4f02
    mov bx, 0x4118
    int 0x10

    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm_start

; --- FUNCIONES AUX ---
read_one_sector:
    pusha
    mov word [dap_off_s2], 0x1000
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap_size_s2
    int 0x13
    popa
    ret

copy_to_high_mem:
    pusha
    mov esi, 0x1000
    mov ecx, 128
.c: a32 lodsd
    db 0x64, 0x67, 0x89, 0x07 ; mov [fs:edi], eax
    a32 add edi, 4
    loop .c
    popa
    ret

print_string:
    mov ah, 0x0e
.l: lodsb
    test al, al
    jz .d
    int 0x10
    jmp .l
.d: ret

print_char:
    mov ah, 0x0e
    int 0x10
    ret

enable_a20:
    mov ax, 0x2401
    int 0x15
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

hang_forever:
    hlt
    jmp hang_forever

; --- DATOS ---
msg_welcome db "BOOT v12", 13, 10, 0
boot_drive db 0
sector_factor dd 1
mem_count_extended dd 0
rsdp_addr_low dd 0
kernel_size dd 0
kernel_lba_current dd 0

align 16
dap_size_s2 db 0x10
dap_res_s2  db 0
dap_count_s2 dw 1
dap_off_s2  dw 0x1000
dap_seg_s2  dw 0x0000
dap_lba_s2  dq 0

gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF ; 0x08: Code 32
    dq 0x00CF92000000FFFF ; 0x10: Data 32
    dq 0x00009A000000FFFF ; 0x18: Code 16
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    ; Paginación simplificada
    mov edi, 0x20000
    mov cr3, edi
    xor eax, eax
    mov ecx, 6144
    rep stosd
    mov dword [0x20000], 0x21003
    mov dword [0x21000], 0x22003
    mov edi, 0x22000
    mov eax, 0x00000083
    mov ecx, 1024
.m: mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .m
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
    lgdt [gdt_ptr_long]
    jmp 0x08:long_mode_start

[bits 64]
long_mode_start:
    mov rsp, 0x9FFF0
    mov rdi, 0x6000
    mov rax, 0xFFFF800000100000
    call rax
    hlt

gdt_start_long:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end_long:
gdt_ptr_long:
    dw gdt_end_long - gdt_start_long - 1
    dq gdt_start_long
