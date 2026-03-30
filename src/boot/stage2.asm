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

    ; Trace: Mensaje de Bienvenida claro
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
    int 0x13

    ; 2. Hardware init: A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Detectar Memoria E820
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

    ; Buscar RSDP (ACPI) en 0xE0000 - 0xFFFFF
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
    ; Probar segmento 0xF000
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

    ; Trace: 'M'
    mov ah, 0x0e
    mov al, 'M'
    int 0x10

    ; 4. Detección de Video
    mov ax, 0x4f01
    mov cx, 0x4118 ; 1024x768x32
    mov di, 0x7000 ; ModeInfoBlock
    int 0x10

    ; 4. Cargar el Kernel
    ; Buffer Seguro: 0x4000:0x0010 (Físico 0x40010). NO está en frontera de 64KB.

    ; Intentamos LBA 128 (HDD) o LBA 32 (ISO 2048-bytes)
    mov dword [dap_lba], 128
    mov dword [dap_lba + 4], 0
.try_read_sb:
    ; Reset DAP con alineación estricta a 0x0000 para VirtualBox SATA
    mov byte [dap], 0x10
    mov byte [dap + 1], 0
    mov word [dap + 2], 1      ; count
    mov word [dap + 4], 0x0000 ; offset 0 bytes
    mov word [dap + 6], 0x5000 ; segment (Buffer 0x50000)

    mov cx, 5
.retry_sb:
    push cx
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    pop cx
    jnc .sb_read_ok

    ; Reset disco en cada fallo
    push ax
    xor ax, ax
    int 0x13
    pop ax
    loop .retry_sb

    ; Si falló 128, probamos 32
    cmp dword [dap_lba], 32
    je disk_error_s2
    mov dword [dap_lba], 32
    jmp .try_read_sb

.sb_read_ok:
    ; Verificar Magic
    mov ax, 0x5000
    mov es, ax
    xor si, si
    cmp dword [es:si], 0xCA121E1 ; Magic en offset 0x00
    je .found_sb

    cmp dword [dap_lba], 32
    je disk_error_s2
    mov dword [dap_lba], 32
    jmp .try_read_sb

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
    mov dword [dap_lba + 4], 0

    ; 16KB = 8 sectores de 2048 o 32 de 512
    mov eax, 32
    xor edx, edx
    div dword [sector_factor]
    mov [dap + 2], ax
    mov word [dap + 4], 0x0000 ; offset 0
    mov word [dap + 6], 0x5100 ; segment (Buffer 0x51000)

    mov cx, 3
.retry_inodes:
    push cx
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    pop cx
    jnc .inodes_ok
    loop .retry_inodes
    jmp disk_error_s2

.inodes_ok:
    push ds
    push es
    mov ax, 0x5100
    mov ds, ax ; DS = Segmento del buffer (0x5100:0x0000)
    xor ax, ax
    mov es, ax ; ES = Segmento del código (0x0000:kernel_name)
    xor si, si ; Offset 0x00
    mov cx, 128
.search_loop:
    push cx
    push si
    mov di, kernel_name
    mov cx, 6
    repe cmpsb
    je .match
    pop si
    pop cx
    add si, 80 ; carleyfs_inode_t = 80 bytes
    loop .search_loop
    pop es
    pop ds
    jmp kernel_not_found_err

.match:
    pop si
    pop cx
    jmp .found_kernel

.found_kernel:
    mov eax, [si + 64] ; size
    mov ebx, [si + 68] ; start sector
    pop es
    pop ds
    xor ax, ax
    mov ds, ax

    mov [kernel_sectors_left_bytes], eax
    mov [kernel_lba_current], ebx
    mov edi, 0x100000

.load_loop:
    xor ax, ax
    mov ds, ax ; Asegurar DS=0 para acceder al DAP
    mov eax, [kernel_lba_current]
    xor edx, edx
    div dword [sector_factor]
    mov [dap_lba], eax
    mov dword [dap_lba + 4], 0

    ; Leer de a 1 sector físico
    mov word [dap + 2], 1
    mov word [dap + 4], 0x0000
    mov word [dap + 6], 0x2000 ; Buffer 0x20000 (Seguro y alineado)

    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_s2

    ; Entrar en Modo Protegido de 32 bits solo para copiar
    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:.pm_copy

[bits 32]
.pm_copy:
    mov bx, 0x10
    mov ds, bx
    mov es, bx

    ; Bytes a copiar = sector_factor * 512
    mov eax, [0x7E00 + (sector_factor - stage2_start)]
    shl eax, 7 ; dwords
    mov ecx, eax
    mov esi, 0x20000
.copy_pm:
    mov eax, [esi]
    mov [edi], eax
    add esi, 4
    add edi, 4
    loop .copy_pm

    ; Volver a Modo Real
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0x00:.real_back_near
.real_back_near:
    ; Salto lejano para recargar CS
    db 0xEA
    dw .real_back
    dw 0x0000

[bits 16]
.real_back:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    sti

    ; Actualizar contadores
    mov eax, [sector_factor]
    add [kernel_lba_current], eax
    shl eax, 9 ; bytes

    cmp [kernel_sectors_left_bytes], eax
    jbe .kernel_ok
    sub [kernel_sectors_left_bytes], eax
    jmp .load_loop

.kernel_ok:
    ; Trace: 'K'
    mov ah, 0x0e
    mov al, 'K'
    int 0x10

    ; 5. Activar Modo Gráfico
    mov ax, 0x4f02
    mov bx, 0x4118
    int 0x10

    ; 6. Salto a Modo Protegido
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm_start

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

kernel_name db "kernel", 0
msg_welcome db "CARLEY BOOTLOADER v6", 13, 10, 0
msg_disk_err db "ERR: DISK ", 0
msg_kernel_err db "ERR: KERNEL NOT FOUND", 0
msg_no_lba db "ERR: NO LBA SUPPORT", 0

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

mem_count_extended dd 0
rsdp_addr_low dd 0
sector_factor dd 1
boot_drive db 0
kernel_sectors_left_bytes dd 0
kernel_lba_current dd 0

align 16
dap: times 16 db 0
dap_lba equ dap + 8

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
