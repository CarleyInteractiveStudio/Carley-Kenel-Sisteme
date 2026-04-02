[bits 16]
[org 0x7E00]

stage2_start:
    ; Intento de escritura directa a memoria de video (no BIOS)
    ; Esto debería mostrar una 'S' en la esquina superior izquierda
    mov ax, 0xB800
    mov es, ax
    mov byte [es:0], 'S'
    mov byte [es:1], 0x0F ; Blanco sobre negro

    hlt
    jmp $
