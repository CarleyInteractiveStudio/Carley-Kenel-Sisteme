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
    mov sp, 0x7c00 ; Pila por debajo del MBR

    ; Guardar unidad de arranque
    mov [boot_drive], dl

    ; 'B' - MBR Iniciado
    mov ax, 0x0e42
    xor bx, bx
    int 0x10

    ; Cargar Stage 2 en 0x1000 (Dirección ultra-segura)
    mov si, dap_stage2
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc jump_to_stage2

    ; Error de lectura 'F'
    mov ax, 0x0e46
    xor bx, bx
    int 0x10
    jmp $

jump_to_stage2:
    ; 'J' - Cargado
    mov ax, 0x0e4a
    xor bx, bx
    int 0x10
    ; '>' - Saltando a 0x1000
    mov ax, 0x0e3e
    int 0x10

    mov dl, [boot_drive]
    jmp 0x0000:0x1000 ; Salto crítico

boot_drive db 0

align 16
dap_stage2:
    db 0x10
    db 0
    dw 31          ; 31 sectores
    dw 0x1000      ; Offset 0x1000
    dw 0x0000      ; Segmento 0
    dq 1           ; LBA 1

times 510-($-$$) db 0
dw 0xaa55
