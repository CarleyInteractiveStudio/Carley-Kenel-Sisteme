[bits 16]
[org 0x7c00]

; --- CARLEY OS MBR v11 (DIAGNOSTIC MODE) ---

start:
    jmp 0:init

init:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [boot_drive], dl

    ; 'B' (MBR Init)
    mov al, 'B'
    call debug_char

    ; Reset disk system
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; 'L' (Loading)
    mov al, 'L'
    call debug_char

    ; Load Stage 2
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jnc .jump_to_stage2

    ; Error loading
    mov al, 'E'
    call debug_char
    jmp .hang

.jump_to_stage2:
    ; 'V' (Verified/About to Jump)
    mov al, 'V'
    call debug_char

    ; --- EXPERIMENTO: PARAR AQUÍ PARA VER SI REINICIA ---
    ; Si el sistema se reinicia aquí, el problema es la BIOS o int 0x10
    ; Si el sistema SE QUEDA QUIETO, el problema era el salto anterior.
    ; Descomenta la siguiente línea para probar el salto:
    ; jmp 0:0x7E00

    hlt ; Por ahora, paramos para confirmar 'BLV'

.hang:
    jmp .hang

debug_char:
    mov ah, 0x0e
    int 0x10
    ; Delay largo
    push cx
    mov cx, 0x000F
.d1:
    push cx
    xor cx, cx
.d2: loop .d2
    pop cx
    loop .d1
    pop cx
    ret

; Datos
boot_drive db 0
align 4
dap:
    db 0x10, 0
    dw 64         ; 32KB
    dw 0x7E00, 0
    dq 1          ; Sector 1

times 510-($-$$) db 0
dw 0xaa55
