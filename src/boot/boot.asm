[bits 16]
[org 0x7c00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Salto largo para asegurar CS = 0
    jmp 0:.next
.next:
    mov [boot_drive], dl

    ; 'B' - MBR OK
    mov ax, 0x0e42
    xor bx, bx
    int 0x10

    ; 1. Verificar extensiones LBA
    mov ah, 0x41
    mov bx, 0x55aa
    int 0x13
    jc .no_lba
    cmp bx, 0xaa55
    jne .no_lba

    ; 'L' - LBA OK
    mov ax, 0x0e4c
    xor bx, bx
    int 0x10

    mov ah, 0x42
    mov si, dap_stage2
    mov dl, [boot_drive]
    int 0x13
    jnc .jump_now

.no_lba:
    ; 'C' - Fallback a CHS
    mov ax, 0x0e43
    xor bx, bx
    int 0x10

    xor ax, ax
    mov es, ax
    mov bx, 0x8000 ; Destino
    mov ax, 0x021f ; Leer 31 sectores
    mov cx, 0x0002 ; Sector 2, Cilindro 0
    mov dh, 0      ; Cabeza 0
    mov dl, [boot_drive]
    int 0x13
    jc .error

.jump_now:
    ; 'J' - Cargado
    mov ax, 0x0e4a
    xor bx, bx
    int 0x10

    mov dl, [boot_drive]
    jmp 0x0000:0x8000

.error:
    ; 'E' - Error
    mov ax, 0x0e45
    xor bx, bx
    int 0x10
    hlt

boot_drive db 0

align 16
dap_stage2:
    db 0x10
    db 0
    dw 31          ; Cantidad sectores
    dw 0x8000      ; Offset
    dw 0x0000      ; Segmento
    dq 1           ; LBA inicial

times 510-($-$$) db 0
dw 0xaa55
