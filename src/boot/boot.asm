[bits 16]
[org 0x7c00]

; --- CARLEY OS MBR (BIOS/HDD/ISO) ---
; Este MBR carga manualmente el Stage 2 para máxima compatibilidad.

start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00 ; Pila segura debajo del MBR

    ; Normalizar CS:IP
    jmp 0x0000:.next
.next:
    mov [boot_drive], dl

    ; Trace: 'B' (MBR)
    mov ax, 0x0e42
    xor bx, bx
    int 0x10

    ; 1. Reset disk system (importante para VirtualBox)
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13

    ; 2. Verificar extensiones LBA
    mov ah, 0x41
    mov bx, 0x55aa
    int 0x13
    jc .no_lba
    cmp bx, 0xaa55
    jne .no_lba

    ; 3. Cargar Stage 2 (Sectores 1 a 64)
    ; Cargamos 64 sectores (32KB) empezando desde el LBA 1
    ; Los colocamos en 0x0000:0x7E00
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jnc .jump_to_stage2

    ; Fallback a CHS si LBA falla
    mov ax, 0x023F ; Leer 63 sectores
    mov cx, 0x0002 ; Cilindro 0, Sector 2
    mov dx, 0x0000 ; Cabeza 0
    mov dl, [boot_drive]
    mov bx, 0x7E00
    int 0x13
    jnc .jump_to_stage2

.disk_error:
    mov si, msg_disk_err
    call print_string
    ; Mostrar código de error en AH
    mov al, ah
    call print_hex
    jmp .hang

.jump_to_stage2:
    ; Trace: 'V' (Stage 2 Loaded)
    mov ax, 0x0e56
    xor bx, bx
    int 0x10

    mov dl, [boot_drive]
    jmp 0x0000:0x7E00

.no_lba:
    ; Intentar lectura CHS si no hay LBA
    mov ax, 0x023F
    mov cx, 0x0002
    mov dx, 0x0000
    mov dl, [boot_drive]
    mov bx, 0x7E00
    int 0x13
    jnc .jump_to_stage2

    mov si, msg_no_lba
    call print_string
    jmp .hang

.hang:
    hlt
    jmp .hang

print_string:
    mov ah, 0x0e
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

print_hex:
    pusha
    mov cx, 2
.loop_hex:
    push ax
    shr al, 4
    and al, 0x0F
    cmp al, 10
    jl .digit
    add al, 7
.digit:
    add al, '0'
    mov ah, 0x0e
    int 0x10
    pop ax
    shl al, 4
    loop .loop_hex
    popa
    ret

; Datos
boot_drive db 0
msg_no_lba db "ERR: LBA", 0
msg_disk_err db "ERR: DISK ", 0

align 4
dap:
    db 0x10       ; Tamaño del DAP
    db 0          ; Reservado
    dw 64         ; Número de sectores a leer (32KB)
    dw 0x7E00     ; Offset de destino
    dw 0x0000     ; Segmento de destino
    dq 1          ; LBA inicial (Sector 1)

times 510-($-$$) db 0
dw 0xaa55
