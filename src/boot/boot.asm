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
    mov ah, 0x0e
    mov al, 'B'
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

    ; Fallback a CHS si LBA falla
    mov ah, 0x02
    mov al, 31          ; Cargar 31 sectores (tras el MBR)
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov bx, 0x7E00      ; Cargar justo tras el MBR
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

jump_to_stage2:
    ; Imprimir 'J' (Jump to Stage 2)
    mov ah, 0x0e
    mov al, 'J'
    int 0x10

    mov dl, [boot_drive]
    jmp 0x0000:0x7E00

disk_error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp $

boot_drive db 0

align 16
dap_stage2:
    db 0x10
    db 0
    dw 31          ; 31 sectores (para que quepan en 16KB totales con el MBR)
    dw 0x7E00      ; Offset 0x7E00
    dw 0x0000      ; Segmento 0
    dq 1           ; Empezar en LBA 1

times 510-($-$$) db 0
dw 0xaa55
