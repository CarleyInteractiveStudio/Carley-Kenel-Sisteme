[bits 16]
[org 0x7E00]

stage2_start:
    ; 1. Normalización total e inmediata
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Trace: 'S' (Stage 2 iniciado)
    mov ax, 0x0e53
    xor bx, bx
    int 0x10

    ; 2. Hardware: A20 y Memoria
    in al, 0x92
    or al, 2
    out 0x92, al

    mov di, 0x9000
    xor ebx, ebx
    mov edx, 0x534D4150
    mov dword [mem_count_extended], 0
.do_e820:
    mov eax, 0xe820
    mov ecx, 24
    int 0x15
    jc .e820_done
    add di, 24
    inc dword [mem_count_extended]
    test ebx, ebx
    jne .do_e820
.e820_done:

    ; Trace: 'E'
    mov al, 'E'
    call print_char_s2

    ; 3. Detección REAL de Video (Esto evita el Error Crítico)
    mov ax, 0x4f01
    mov cx, 0x118   ; 1024x768x32
    mov di, 0x7000  ; Buffer para ModeInfo
    int 0x10

    ; 4. Cargar el Kernel desde el disco (LBA 64+)
    ; Leer Superbloque (LBA 64)
    mov dword [dap_lba], 64
    mov word [dap_count], 1
    mov word [dap_segment], 0x1000
    mov word [dap_offset], 0x0000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_s2

    ; Leer Inodos (LBA 65)
    mov dword [dap_lba], 65
    mov word [dap_count], 32
    mov word [dap_segment], 0x1020
    mov ah, 0x42
    int 0x13
    jc disk_error_s2

    ; Buscar "kernel"
    mov ax, 0x1020
    mov ds, ax
    xor si, si
    mov cx, 64
.search_loop:
    push cx
    mov di, kernel_name
    xor ax, ax
    mov es, ax
    mov cx, 6
    repe cmpsb
    pop cx
    je .found_kernel
    add si, 80
    loop .search_loop
    jmp disk_error_s2

.found_kernel:
    mov eax, [si + 64] ; size
    xor bx, bx
    mov es, bx
    mov [es:kernel_sectors_left_bytes], eax
    mov eax, [si + 68] ; start_sector
    mov [es:kernel_lba_current], eax
    mov edi, 0x100000 ; Destino final (1MB)

    ; Unreal Mode para escribir en 1MB
    push ds
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:.unreal
[bits 32]
.unreal:
    mov bx, 0x10
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0x18:.unreal_done
[bits 16]
.unreal_done:
    pop ds
    xor ax, ax
    mov es, ax

.load_kernel:
    mov eax, [kernel_lba_current]
    mov [dap_lba], eax
    mov word [dap_count], 64
    mov word [dap_segment], 0x4000
    mov ah, 0x42
    int 0x13
    jc disk_error_s2

    mov ecx, (64 * 512) / 4
    mov esi, 0x40000
.copy_to_1mb:
    mov eax, [gs:esi]
    mov [gs:edi], eax
    add esi, 4
    add edi, 4
    loop .copy_to_1mb

    add dword [kernel_lba_current], 64
    cmp dword [kernel_sectors_left_bytes], (64 * 512)
    jbe .kernel_ok
    sub dword [kernel_sectors_left_bytes], (64 * 512)
    jmp .load_kernel

.kernel_ok:
    ; Trace: 'K'
    mov al, 'K'
    call print_char_s2

    ; 5. Activar Modo Gráfico
    mov ax, 0x4f02
    mov bx, 0x4118
    int 0x10

    ; 6. Paso a Modo Protegido y Long Mode
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm_start

print_char_s2:
    pusha
    mov ah, 0x0e
    xor bx, bx
    int 0x10
    popa
    ret

disk_error_s2:
    mov al, 'F'
    call print_char_s2
    hlt

kernel_name db "kernel", 0

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; 7. Paginación de 64 bits
    mov edi, 0x20000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd

    mov dword [0x20000], 0x21003
    mov dword [0x20000 + 256*8], 0x21003
    mov dword [0x21000], 0x22003
    mov dword [0x21008], 0x23003
    mov dword [0x21010], 0x24003
    mov dword [0x21018], 0x25003

    mov edi, 0x22000
    mov eax, 0x00000083
    mov ecx, 2048
.map_loop:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_loop

    ; 8. Long Mode
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
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x9FFF0

    ; 9. Pasar Boot Info al Kernel
    ; Dirección de video real obtenida en el paso 3
    mov eax, [0x7000 + 40]
    mov [0x6000], rax
    mov dword [0x6008], 1024
    mov dword [0x600c], 768
    mov qword [0x6010], 0x9000
    mov eax, [mem_count_extended]
    mov [0x6018], eax

    mov rdi, 0x6000
    mov rax, 0xFFFF800000100000
    call rax
    hlt

; --- Datos ---
mem_count_extended dd 0
boot_drive db 0
kernel_sectors_left_bytes dd 0
kernel_lba_current dd 0

align 4
dap:
    db 0x10, 0
dap_count: dw 0
dap_offset: dw 0
dap_segment: dw 0
dap_lba: dq 0

gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00009A000000FFFF
    dq 0x000092000000FFFF
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt_start_long:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end_long:
gdt_ptr_long:
    dw gdt_end_long - gdt_start_long - 1
    dq gdt_start_long
