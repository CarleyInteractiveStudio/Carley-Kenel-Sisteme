[bits 16]
[org 0x7E00]

stage2_start:
    ; 1. Normalización total de registros y estado
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00 ; Pila debajo del MBR

    mov [boot_drive], dl

    ; Trace: Mensaje de Bienvenida
    mov si, msg_welcome
    call print_string

    ; 2. Verificar soporte LBA
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]
    int 0x13
    jnc .lba_ok
    mov si, msg_no_lba
    call print_string
    jmp hang_forever
.lba_ok:

    ; Reset disco
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; 3. Hardware init: A20
    call enable_a20
    mov al, 'A'
    call print_char

    ; 3.1 Entrar en Unreal Mode (Cargar FS/GS con límite de 4GB)
    cli
    push ds
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; Salto lejano a un segmento de 16 bits en PM para resetear el descriptor cache
    jmp 0x18:.pm_temp

.pm_temp:
    mov ax, 0x10 ; Selector de datos (límite 4GB)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov eax, cr0
    and al, 0xFE
    mov cr0, eax

    ; Salto lejano para volver a modo real completamente
    jmp 0x0000:.real_temp

.real_temp:
    pop ds
    xor ax, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov al, 'U'
    call print_char

    ; Detectar Memoria E820
    mov di, 0x9000
    xor ebx, ebx
    mov dword [mem_count_extended], 0
.do_e820:
    mov eax, 0xe820
    mov ecx, 24
    mov edx, 0x534D4150
    int 0x15
    jc .e820_done
    cmp eax, 0x534D4150
    jne .e820_done
    add di, 24
    inc dword [mem_count_extended]
    test ebx, ebx
    jne .do_e820
.e820_done:
    mov al, 'M'
    call print_char

    ; Buscar RSDP (ACPI)
    mov dword [rsdp_addr_low], 0
    mov ax, 0xE000
    mov es, ax
    xor di, di
.search_rsdp:
    cmp dword [es:di], 'RSD '
    jne .next_rsdp
    cmp dword [es:di+4], 'PTR '
    je .found_rsdp
.next_rsdp:
    add di, 16
    jnz .search_rsdp
    mov ax, es
    cmp ax, 0xF000
    je .rsdp_done
    mov ax, 0xF000
    mov es, ax
    jmp .search_rsdp

.found_rsdp:
    xor eax, eax
    mov ax, es
    shl eax, 4
    movzx edx, di
    add eax, edx
    mov [rsdp_addr_low], eax
.rsdp_done:
    xor ax, ax
    mov es, ax
    mov al, 'R'
    call print_char

    ; 4. Obtener Información de Video (VBE)
    mov ax, 0x4f01
    mov cx, 0x4118 ; 1024x768x32
    mov di, 0x7000 ; ModeInfoBlock
    int 0x10
    mov al, 'G'
    call print_char

    ; 5. Cargar Superbloque
    mov dword [dap_lba], 128
    mov dword [dap_lba + 4], 0
    call read_one_sector
    jnc .check_magic_sb

    mov dword [dap_lba], 32
    mov dword [dap_lba + 4], 0
    call read_one_sector
    jc disk_error_s2

.check_magic_sb:
    xor ax, ax
    mov es, ax
    mov si, 0x1000
    cmp dword [es:si], 0xCA121E1
    je .found_sb

    cmp dword [dap_lba], 32
    je disk_error_s2
    mov dword [dap_lba], 32
    call read_one_sector
    jc disk_error_s2
    mov si, 0x1000
    cmp dword [es:si], 0xCA121E1
    jne disk_error_s2

.found_sb:
    mov dword [sector_factor], 1
    cmp dword [dap_lba], 32
    jne .read_inodes
    mov dword [sector_factor], 4

.read_inodes:
    mov eax, 132
    xor edx, edx
    div dword [sector_factor]
    mov [dap_lba], eax

    mov edi, 0x30000
    mov eax, 32
    xor edx, edx
    div dword [sector_factor]
    mov ecx, eax
.inode_loop:
    push ecx
    call read_one_sector
    jc disk_error_s2
    call copy_to_high_mem
    inc dword [dap_lba]
    mov eax, [sector_factor]
    shl eax, 9
    add edi, eax
    pop ecx
    loop .inode_loop

    ; Buscar Kernel
    mov ax, 0x3000
    mov ds, ax
    xor si, si
    mov cx, 128
.search_loop:
    push cx
    push si
    mov di, kernel_name_str
    mov cx, 6
    push es
    xor ax, ax
    mov es, ax
    repe cmpsb
    pop es
    je .match
    pop si
    pop cx
    add si, 80
    loop .search_loop
    jmp kernel_not_found_err

.match:
    pop si
    pop cx
    mov eax, [si + 64] ; size
    mov ebx, [si + 68] ; start sector
    xor ax, ax
    mov ds, ax
    mov [kernel_size], eax
    mov [kernel_lba_current], ebx

    ; Cargar Kernel a 0x100000
    mov edi, 0x100000

.load_kernel_loop:
    mov eax, [kernel_lba_current]
    xor edx, edx
    div dword [sector_factor]
    mov [dap_lba], eax

    call read_one_sector
    jc disk_error_s2
    call copy_to_high_mem

    mov eax, [sector_factor]
    add [kernel_lba_current], eax
    shl eax, 9

    cmp [kernel_size], eax
    jbe .kernel_loaded
    sub [kernel_size], eax
    add edi, eax
    jmp .load_kernel_loop

.kernel_loaded:
    mov al, 'K'
    call print_char

    ; Verificar Magic del Kernel
    mov eax, [fs:0x100005]
    cmp eax, 0xC0DEB007
    je .magic_ok
    mov al, 'M'
    call print_char
    jmp hang_forever
.magic_ok:
    mov al, '!'
    call print_char

    ; 6. Modo Gráfico
    mov ax, 0x4f02
    mov bx, 0x4118 ; 1024x768x32
    int 0x10

    ; 7. Salto Final a Modo Protegido
    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm_start

; --- FUNCIONES DE APOYO ---

read_one_sector:
    pusha
    mov byte [dap_size], 0x10
    mov byte [dap_res], 0
    mov word [dap_count], 1
    mov word [dap_off], 0x1000
    mov word [dap_seg], 0x0000
    mov cx, 5
.retry:
    push cx
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap_size
    int 0x13
    pop cx
    jnc .ok
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13
    loop .retry
    popa
    stc
    ret
.ok:
    mov ah, 0x0e
    mov al, '.'
    int 0x10
    popa
    clc
    ret

copy_to_high_mem:
    pusha
    push ds
    xor ax, ax
    mov ds, ax
    mov esi, 0x1000
    mov eax, [sector_factor]
    shl eax, 9
    mov ecx, eax
    shr ecx, 2
.copy_loop:
    a32 lodsd
    db 0x64, 0x67, 0x89, 0x07 ; mov [fs:edi], eax
    a32 add edi, 4
    loop .copy_loop
    pop ds
    popa
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

print_hex:
    pusha
    mov cx, 2
.loop:
    push ax
    shr al, 4
    and al, 0x0F
    cmp al, 10
    jl .digit
    add al, 7
.digit:
    add al, '0'
    mov ah, 0x0e
    int 0x10
    pop ax
    shl al, 4
    loop .loop
    popa
    ret

print_char:
    mov ah, 0x0e
    int 0x10
    ; Serial output
    push dx
    mov dx, 0x3f8
    out dx, al
    pop dx
    ret

enable_a20:
    mov ax, 0x2401
    int 0x15
    jnc .done
    in al, 0x92
    or al, 2
    out 0x92, al
.done:
    ret

disk_error_s2:
    mov si, msg_disk_err
    call print_string
    mov al, ah
    call print_hex
    jmp hang_forever

kernel_not_found_err:
    mov si, msg_kernel_err
    call print_string
hang_forever:
    hlt
    jmp hang_forever

; --- DATOS ---
msg_welcome db "CARLEY BOOTLOADER v9", 13, 10, 0
msg_disk_err db "ERR: DISK ", 0
msg_kernel_err db "ERR: KERNEL NOT FOUND", 0
msg_no_lba db "ERR: NO LBA SUPPORT", 0
kernel_name_str db "kernel", 0

boot_drive db 0
sector_factor dd 1
mem_count_extended dd 0
rsdp_addr_low dd 0
kernel_size dd 0
kernel_lba_current dd 0

align 16
dap_size db 0x10
dap_res  db 0
dap_count dw 1
dap_off  dw 0x1000
dap_seg  dw 0x0000
dap_lba  dq 0

gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF ; 0x08: Code 32-bit
    dq 0x00CF92000000FFFF ; 0x10: Data 32-bit (4GB)
    dq 0x00009A000000FFFF ; 0x18: Code 16-bit
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

    mov edi, 0x20000
    mov cr3, edi
    xor eax, eax
    mov ecx, 6144
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
.map:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map

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

    mov eax, [0x7000 + 40]
    mov [0x6000], rax
    mov dword [0x6008], 1024
    mov dword [0x600c], 768
    mov qword [0x6010], 0x9000
    mov eax, [mem_count_extended]
    mov [0x6018], eax
    mov eax, [rsdp_addr_low]
    mov [0x601C], rax

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
