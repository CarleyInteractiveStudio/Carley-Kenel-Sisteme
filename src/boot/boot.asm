[bits 16]
[org 0x7c00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Guardar el número de unidad de arranque pasado por BIOS en DL
    mov [boot_drive], dl

    ; Pasar el numero de unidad en DL al Stage 2
    mov dl, [boot_drive]

    ; Reiniciar disco
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; Cargar Stage 2 usando LBA extensions (más compatible con ISOs y discos modernos)
    mov si, dap_stage2
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jnc jump_to_stage2

    ; Fallback a CHS si LBA falla
    mov dl, [boot_drive]
    mov ah, 0x02
    mov al, 32
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov bx, 0x8000
    int 0x13
    jc disk_error

jump_to_stage2:
    ; Saltar al Stage 2
    jmp 0x8000

disk_error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp $

boot_drive db 0

align 4
dap_stage2:
    db 0x10
    db 0
    dw 32          ; 32 sectores
    dw 0x8000      ; Offset 0x8000
    dw 0x0000      ; Segmento 0
    dq 1           ; Empezar en LBA 1 (justo tras el MBR)

times 510-($-$$) db 0
dw 0xaa55
