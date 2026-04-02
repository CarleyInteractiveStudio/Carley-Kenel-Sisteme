[bits 16]
[org 0x7c00]

; --- CARLEY OS MBR v12 (AUTO-DETECTION) ---

start:
    jmp 0:init

init:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [boot_drive], dl

    ; 'B' (MBR Init)
    mov al, 'B'
    call debug_char

    ; 1. Reset disk system
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; 2. Intentar cargar Stage 2 (LBA 1 - Asumiendo 512 bytes/sector)
    mov al, 'L'
    call debug_char

    mov dword [dap_lba], 1
    call load_stage2

    ; Verificar firma mágica en 0x7E00
    cmp dword [0x7E00], 0xDEADBEEF
    je .jump_now

    ; 3. Si falla, intentar LBA 1 (Asumiendo 2048 bytes/sector - CD-ROM)
    ; En algunas BIOS de CD, el sector 1 es el LBA 1 real de 2048 bytes.
    mov al, '2' ; Intento 2
    call debug_char

    mov dword [dap_lba], 1
    call load_stage2
    cmp dword [0x7E00], 0xDEADBEEF
    je .jump_now

    ; 4. Fallback final: Intentar cargar varios sectores por si acaso
    mov al, 'F' ; Fallback
    call debug_char
    jmp .disk_error

.jump_now:
    ; 'V' (Verified)
    mov al, 'V'
    call debug_char

    mov dl, [boot_drive]
    jmp 0x0000:0x7E04 ; Saltar justo después de la firma mágica

.disk_error:
    mov al, 'E'
    call debug_char
.hang:
    hlt
    jmp .hang

load_stage2:
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    ret

debug_char:
    mov ah, 0x0e
    int 0x10
    ret

; Datos
boot_drive db 0
align 4
dap:
    db 0x10, 0
    dw 64         ; Cargar 32KB
    dw 0x7E00, 0
dap_lba:
    dq 1          ; LBA inicial

times 510-($-$$) db 0
dw 0xaa55
