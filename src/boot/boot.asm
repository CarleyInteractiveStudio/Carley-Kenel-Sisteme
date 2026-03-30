[bits 16]
[org 0x7c00]

; --- CARLEY OS MBR (ARCH: ISO/HDD) ---
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

    ; Trace: 'B' (MBR)
    mov ax, 0x0e42
    xor bx, bx
    int 0x10

    ; IMPORTANTE: En una ISO, la BIOS ya carga los primeros 32 sectores (16KB)
    ; en memoria RAM empezando en 0x7C00. Por lo tanto, el Stage 2 ya está
    ; en la dirección 0x7E00. Saltamos directamente.
    mov dl, [boot_drive]
    jmp 0x0000:0x7E00

boot_drive db 0
times 510-($-$$) db 0
dw 0xaa55
