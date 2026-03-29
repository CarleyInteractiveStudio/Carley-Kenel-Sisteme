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

    ; Guardar el número de unidad de arranque
    mov [boot_drive], dl

    ; Imprimir 'B' (Bootloader started)
    mov ax, 0x0e42
    xor bx, bx
    int 0x10

    ; Reiniciar disco
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; Cargar Stage 2 usando LBA extensions
    mov si, dap_stage2
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jnc jump_to_stage2

    ; Imprimir 'F' (LBA failed)
    mov ax, 0x0e46
    xor bx, bx
    int 0x10

    ; Fallback a CHS si LBA falla
    mov ah, 0x02
    mov al, 31          ; Cargar 31 sectores
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov bx, 0x7E00      ; Offset 0x7E00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

jump_to_stage2:
    ; Imprimir 'J' (Jump)
    mov ax, 0x0e4a
    xor bx, bx
    int 0x10

    ; Imprimir '>'
    mov ax, 0x0e3e
    xor bx, bx
    int 0x10

    mov dl, [boot_drive]
    jmp 0x0000:0x7E00

disk_error:
    mov ax, 0x0e45 ; 'E'
    xor bx, bx
    int 0x10
    jmp $

boot_drive db 0

align 16
dap_stage2:
    db 0x10
    db 0
    dw 31          ; 31 sectores
    dw 0x7E00      ; Offset 0x7E00
    dw 0x0000      ; Segmento 0
    dq 1           ; Empezar en LBA 1

times 510-($-$$) db 0
dw 0xaa55
