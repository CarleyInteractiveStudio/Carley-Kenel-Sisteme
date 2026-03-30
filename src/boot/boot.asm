[bits 16]
[org 0x7c00]

; --- CARLEY OS MBR (ARQUITECTURA ISO/HDD) ---
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Normalizar CS a 0
    jmp 0x0000:.next
.next:
    mov [boot_drive], dl

    ; Imprimir 'B'
    mov ax, 0x0e42
    xor bx, bx
    int 0x10

    ; IMPORTANTE: En una ISO con -boot-load-size 32, el Stage 2 ya está en RAM.
    ; Se encuentra exactamente en 0x7E00 (offset 512 del MBR).
    ; No re-leemos el disco para evitar conflictos con el driver de CD de la BIOS.
    jmp 0x0000:0x7E00

boot_drive db 0
times 510-($-$$) db 0
dw 0xaa55
