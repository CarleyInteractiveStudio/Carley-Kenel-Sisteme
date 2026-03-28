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

    ; Cargar Stage 2 (suponemos que está justo después del MBR)
    ; Cargamos 32 sectores (16KB aprox) para estar seguros
    mov ah, 0x02
    mov al, 32          ; Sectores a leer
    mov ch, 0           ; Cilindro 0
    mov dh, 0           ; Cabeza 0
    mov cl, 2           ; Sector 2 (el sector 1 es este MBR)
    mov bx, 0x8000      ; Cargar en 0x0000:0x8000
    int 0x13
    jc disk_error

    ; Saltar al Stage 2
    jmp 0x8000

disk_error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp $

boot_drive db 0

times 510-($-$$) db 0
dw 0xaa55
