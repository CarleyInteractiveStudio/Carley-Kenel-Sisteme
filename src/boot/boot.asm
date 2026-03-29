[bits 16]
[org 0x7c00]

start:
    jmp 0x0000:normalize_cs
normalize_cs:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [boot_drive], dl

    ; 'B' - MBR OK
    mov ax, 0x0e42
    xor bx, bx
    int 0x10

    ; 1. Verificar extensiones LBA
    mov ah, 0x41
    mov bx, 0x55aa
    mov dl, [boot_drive]
    int 0x13
    jc no_lba
    cmp bx, 0xaa55
    jne no_lba

    ; 'L' - LBA OK
    mov ax, 0x0e4c
    xor bx, bx
    int 0x10

    ; 2. Cargar Stage 2 (31 sectores) a 0x1000
    mov si, dap_stage2
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; 'J' - Cargado OK
    mov ax, 0x0e4a
    xor bx, bx
    int 0x10
    jmp jump_to_stage2

no_lba:
    ; Fallback a lectura CHS antigua
    mov ax, 0x021f ; Leer 31 sectores
    mov cx, 0x0002 ; Cilindro 0, Sector 2
    mov dh, 0      ; Cabeza 0
    mov bx, 0x1000 ; Destino
    mov es, bx
    xor bx, bx
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    push cs
    pop es

jump_to_stage2:
    ; '>' - Saltando
    mov ax, 0x0e3e
    xor bx, bx
    int 0x10

    mov dl, [boot_drive]
    jmp 0x0000:0x1000

disk_error:
    ; 'F' - Fallo
    mov ax, 0x0e46
    xor bx, bx
    int 0x10
    hlt

boot_drive db 0

align 16
dap_stage2:
    db 0x10
    db 0
    dw 31          ; Sectores
    dw 0x1000      ; Offset
    dw 0x0000      ; Segmento
    dq 1           ; LBA 1

times 510-($-$$) db 0
dw 0xaa55
