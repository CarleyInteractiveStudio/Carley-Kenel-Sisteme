[bits 16]
[org 0x7E00]

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov [boot_drive], dl

    ; 1. Habilitar la línea A20
    in al, 0x92
    or al, 2
    out 0x92, al

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
    test ebx, ebx
    jne do_e820
e820_done:

    ; 3. Verificación de Hardware (CPUID Long Mode)
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_long_mode

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
    ; Leer Superbloque (LBA 64)
    mov dword [dap_lba], 64
    mov word [dap_count], 1
    mov word [dap_segment], 0x0700 ; 0x7000
    mov word [dap_offset], 0x0000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_stage2

    mov eax, [0x7000]
    cmp eax, 0xCA121E1
    jne disk_error_stage2

    ; Leer Tabla de Inodos (LBA 65, 32 sectores) a 0x7200
    mov dword [dap_lba], 65
    mov word [dap_count], 32
    mov word [dap_segment], 0x0720
    mov si, dap
    int 0x13
    jc disk_error_stage2

    ; Buscar "kernel"
    mov si, 0x7200
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
    mov dword [kernel_sectors_left_bytes], eax
    mov eax, [si + 68] ; start
    mov dword [kernel_lba_current], eax
    mov edi, 0x100000

    ; Entrar en Unreal Mode para cargar directamente a 1MB
    push ds
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
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0x18:unreal_done
[bits 16]
unreal_done:
    pop ds

load_kernel_loop:
    mov eax, [kernel_lba_current]
    mov [dap_lba], eax
    mov word [dap_count], 64
    mov word [dap_segment], 0x2000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_stage2

    mov ecx, (64 * 512) / 4
    mov esi, 0x20000
    rep movsd

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
    mov si, msg_video_info
    call print_string

    ; Intentar 1024x768x32 (0x118)
    mov ax, 0x4f01
    mov cx, 0x118
    mov di, 0x7000
    int 0x10
    cmp ax, 0x004f
    je video_ok

    ; Fallback 800x600x32 (0x115)
    mov cx, 0x115
    int 0x10

video_ok:
    mov ax, 0x4f02
    mov bx, cx
    or bx, 0x4000 ; LFB
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
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

msg_header db "--- CARLEY OS BOOTLOADER v2.0 ---", 13, 10, 0
msg_option1 db "[1] Iniciar Carley OS", 13, 10, 0
msg_option2 db "[2] Reiniciar", 13, 10, 0
msg_countdown db 13, "Iniciando en: ", 0
msg_loading db 13, 10, "Cargando sistema...", 13, 10, 0
msg_video_info db "Configurando video...", 13, 10, 0
msg_err_cpu db "ERROR: CPU no soporta 64-bit.", 0
msg_err_disk db "ERROR: Kernel no encontrado en CarleyFS.", 0

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; 8. Paginación (Higher Half Mappings)
    mov edi, 0x20000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd

    ; PML4[0] -> Identity Map (0-512GB)
    ; PML4[256] -> HHDM (0xFFFF800000000000)
    mov dword [0x20000], 0x21003
    mov dword [0x20000 + 256*8], 0x21003

    mov dword [0x21000], 0x22003 ; PDPT[0]
    mov dword [0x21008], 0x23003 ; PDPT[1]
    mov dword [0x21010], 0x24003 ; PDPT[2]
    mov dword [0x21018], 0x25003 ; PDPT[3]

    mov edi, 0x22000
    mov eax, 0x00000083 ; 2MB pages
    mov ecx, 2048
map_loop:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop map_loop

    ; 9. Long Mode
    mov eax, cr4
    or eax, 1 << 5 ; PAE
    mov cr4, eax

    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8 ; LME
    wrmsr

    mov eax, cr0
    or eax, 1 << 31 ; Paging
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

    ; Boot Info en 0x6000
    mov eax, [0x7000 + 40] ; LFB
    mov [0x6000], rax
    mov dword [0x6008], 1024
    mov dword [0x600c], 768
    mov qword [0x6010], 0x9000
    mov eax, [mem_count_extended]
    mov [0x6018], eax

    ; Buscar RSDP para pasarlo al Kernel
    xor rbx, rbx
    mov rsi, 0xE0000
.search_rsdp:
    mov rax, [rsi]
    mov rdx, 0x2052545020445352 ; "RSD PTR "
    cmp rax, rdx
    je .found_rsdp
    add rsi, 16
    cmp rsi, 0xFFFFF
    jb .search_rsdp
    jmp .done_rsdp
.found_rsdp:
    mov [0x601C], rsi ; rsdp_address
.done_rsdp:

    mov rdi, 0x6000
    ; El Kernel está linkeado en 0xFFFF800000100000
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
    dq 0x00CF9A000000FFFF ; Code 32
    dq 0x00CF92000000FFFF ; Data 32
    dq 0x00009A000000FFFF ; Code 16
    dq 0x000092000000FFFF ; Data 16
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt_start_long:
    dq 0
    dq 0x00AF9A000000FFFF ; Code 64
    dq 0x00CF92000000FFFF ; Data 64
gdt_end_long:
gdt_ptr_long:
    dw gdt_end_long - gdt_start_long - 1
    dq gdt_start_long
