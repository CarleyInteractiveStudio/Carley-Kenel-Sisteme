[bits 16]
[org 0x8000]

stage2_start:
    ; Imprimir debug ultra-temprano: '!'
    mov ax, 0x0e21 ; '!'
    xor bx, bx
    int 0x10

    ; '1', '2', '3'
    mov al, '1'
    int 0x10
    mov al, '2'
    int 0x10
    mov al, '3'
    int 0x10

    jmp actual_start
    nop

actual_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov [boot_drive], dl

    ; Imprimir 'S' (Stage 2 started)
    mov ah, 0x0e
    mov al, 'S'
    int 0x10

    ; 1. Habilitar la línea A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Imprimir 'A' (A20 done)
    mov ah, 0x0e
    mov al, 'A'
    int 0x10

    ; 2. Detectar Mapa de Memoria BIOS (E820)
    mov di, 0x9000
    xor ebx, ebx
    mov edx, 0x534D4150
    mov dword [mem_count_extended], 0
do_e820:
    mov eax, 0xe820
    mov ecx, 24
    int 0x15
    jc e820_done
    cmp eax, 0x534D4150
    jne e820_done
    add di, 24
    inc dword [mem_count_extended]
    cmp dword [mem_count_extended], 128 ; Límite de seguridad
    jae e820_done
    test ebx, ebx
    jne do_e820
e820_done:
    ; Imprimir 'E' (E820 done)
    mov ah, 0x0e
    mov al, 'E'
    int 0x10

    ; 3. Verificación de Hardware (CPUID Long Mode)
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_long_mode

    ; Imprimir 'C' (CPU 64-bit verified)
    mov ah, 0x0e
    mov al, 'C'
    int 0x10

    ; 4. Menú de Arranque
    call clear_screen
    mov si, msg_header
    call print_string

    mov ah, 0x0e
    mov al, 'M'
    int 0x10
    mov si, msg_option1
    call print_string
    mov si, msg_option2
    call print_string

    mov cx, 5 ; 5 segundos
boot_menu_loop:
    mov si, msg_countdown
    call print_string
    mov al, cl
    add al, '0'
    mov ah, 0x0e
    int 0x10

    mov ah, 0x01
    int 0x16
    jnz handle_key

    mov ah, 0x86
    mov cx, 0x000F
    mov dx, 0x4240
    int 0x15

    loop boot_menu_loop
    jmp start_loading

handle_key:
    mov ah, 0x00
    int 0x16
    cmp al, '1'
    je start_loading
    cmp al, '2'
    je reboot_system
    jmp boot_menu_loop

no_long_mode:
    mov si, msg_err_cpu
    call print_string
    jmp $

reboot_system:
    jmp 0xFFFF:0x0000

start_loading:
    call clear_screen
    mov si, msg_loading
    call print_string

    mov ah, 0x0e
    mov al, 'L'
    int 0x10

    ; 5. Cargar el Kernel desde CarleyFS
    ; Usar buffer en 0x1000:0x0000 (0x10000) para evitar solapamientos con el cargador
    ; Leer Superbloque (LBA 64)
    mov dword [dap_lba], 64
    mov word [dap_count], 1
    mov word [dap_segment], 0x1000 ; 0x10000 físico
    mov word [dap_offset], 0x0000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_stage2

    mov ax, 0x1000
    mov gs, ax
    mov eax, [gs:0]
    cmp eax, 0xCA121E1
    jne disk_error_stage2

    ; Leer Tabla de Inodos (LBA 65, 32 sectores) a 0x10200 físico
    mov dword [dap_lba], 65
    mov word [dap_count], 32
    mov word [dap_segment], 0x1020
    mov word [dap_offset], 0x0000
    mov si, dap
    int 0x13
    jc disk_error_stage2

    ; Buscar "kernel" en el buffer de inodos (DS:0)
    mov ax, 0x1020
    mov ds, ax
    xor si, si
    mov cx, 64
search_kernel:
    push cx
    mov di, kernel_name
    mov cx, 6
    push si
    repe cmpsb
    pop si
    pop cx
    je found_kernel_inode
    add si, 80
    loop search_kernel
    jmp disk_error_stage2

found_kernel_inode:
    mov eax, [si + 64] ; size
    ; Volver a segmento 0 para guardar variables y usar Unreal Mode
    xor bx, bx
    mov es, bx
    mov [es:kernel_sectors_left_bytes], eax
    mov eax, [si + 68] ; start
    mov [es:kernel_lba_current], eax
    mov edi, 0x100000

    ; Entrar en Unreal Mode para cargar directamente a 1MB
    push ds
    xor ax, ax
    mov ds, ax
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:unreal_setup
[bits 32]
unreal_setup:
    mov bx, 0x10
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0x18:unreal_done
[bits 16]
unreal_done:
    xor ax, ax
    mov ds, ax
    mov es, ax

load_kernel_loop:
    mov eax, [kernel_lba_current]
    mov [dap_lba], eax
    mov word [dap_count], 64
    mov word [dap_segment], 0x4000 ; Buffer temporal en 0x40000
    mov word [dap_offset], 0x0000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_stage2

    ; Copiar usando GS (Unreal Mode) para evitar colisiones
    mov ecx, (64 * 512) / 4
    mov esi, 0x40000
.inner_copy:
    mov eax, [gs:esi]
    mov [gs:edi], eax
    add esi, 4
    add edi, 4
    loop .inner_copy

    add dword [kernel_lba_current], 64
    cmp dword [kernel_sectors_left_bytes], (64 * 512)
    jbe kernel_loaded
    sub dword [kernel_sectors_left_bytes], (64 * 512)
    jmp load_kernel_loop

kernel_loaded:
    xor ax, ax
    mov ds, ax
    mov es, ax

    ; 6. Configurar Modo de Video VBE
    mov ax, 0x4f02
    mov bx, 0x4118 ; 1024x768x32 LFB
    int 0x10

    mov ah, 0x0e
    mov al, 'V'
    int 0x10

    ; 7. Paso a Modo Protegido
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_start

disk_error_stage2:
    mov si, msg_err_disk
    call print_string
    hlt

clear_screen:
    mov ax, 0x03
    int 0x10
    ret

print_string:
    mov ah, 0x0e
    xor bx, bx
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

msg_header db "--- CARLEY OS BOOTLOADER v2.3 ---", 13, 10, 0
msg_option1 db "[1] Iniciar Carley OS", 13, 10, 0
msg_option2 db "[2] Reiniciar", 13, 10, 0
msg_countdown db 13, "Iniciando en: ", 0
msg_loading db 13, 10, "Cargando sistema...", 13, 10, 0
msg_err_cpu db "ERROR: CPU no soporta 64-bit.", 0
msg_err_disk db "ERROR: Archivo no encontrado.", 0

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; 8. Paginación
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
map_loop:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop map_loop

    ; 9. Long Mode
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
    mov rsp, 0x9FFFF

    ; Boot Info
    mov rax, 0xFD000000
    mov qword [0x6000], rax
    mov dword [0x6008], 1024
    mov dword [0x600c], 768
    mov qword [0x6010], 0x9000
    mov eax, [mem_count_extended]
    mov [0x6018], eax

    mov rdi, 0x6000
    mov rax, 0xFFFF800000100000
    call rax
    jmp $

mem_count_extended dd 0
boot_drive db 0
kernel_sectors_left_bytes dd 0
kernel_name db "kernel", 0
kernel_lba_current dd 0

align 4
dap:
    db 0x10
    db 0
dap_count:
    dw 0
dap_offset:
    dw 0x0000
dap_segment:
    dw 0x0000
dap_lba:
    dq 0

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
