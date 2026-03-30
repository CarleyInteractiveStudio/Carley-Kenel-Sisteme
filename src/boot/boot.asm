[bits 16]
[org 0x7c00]

; --- CARLEY OS MBR (EL TORITO COMPATIBLE) ---
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00 ; Pila segura

    ; Normalizar CS
    jmp 0x0000:.next
.next:
    mov [boot_drive], dl

    ; Trace: 'B'
    mov ax, 0x0e42
    xor bx, bx
    int 0x10

    ; En una ISO, el Stage 2 ya está en memoria (0x7E00). Saltamos directamente.
    mov dl, [boot_drive]
    jmp 0x0000:0x7E00

boot_drive db 0
times 510-($-$$) db 0
dw 0xaa55
