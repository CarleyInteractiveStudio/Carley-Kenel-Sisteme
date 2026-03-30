[bits 16]
[org 0x7c00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    ; Asegurar CS = 0 mediante un salto largo
    jmp 0:.next
.next:
    mov [boot_drive], dl

    ; 'B'
    mov al, 'B'
    call print_char

    ; 1. Verificar LBA
    mov ah, 0x41
    mov bx, 0x55aa
    int 0x13
    jc .no_lba
    cmp bx, 0xaa55
    jne .no_lba

    ; 'L'
    mov al, 'L'
    call print_char

    mov ah, 0x42
    mov si, dap_stage2
    mov dl, [boot_drive]
    int 0x13
    jnc .jump_now

.no_lba:
    ; 'C' (CHS Fallback)
    mov al, 'C'
    call print_char

    xor ax, ax
    mov es, ax
    mov bx, 0x8000
    mov ax, 0x0210
    mov cx, 0x0002
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc .error

.jump_now:
    ; 'J'
    mov al, 'J'
    call print_char

    ; '>'
    mov al, '>'
    call print_char

    mov dl, [boot_drive]
    ; SALTO CRÍTICO: Far Jump para normalizar CS:IP
    jmp 0x0000:0x8000

.error:
    mov al, 'E'
    call print_char
    hlt

; Función robusta para imprimir
print_char:
    pusha
    mov ah, 0x0e
    xor bx, bx
    int 0x10
    popa
    ret

boot_drive db 0

align 16
dap_stage2:
    db 0x10
    db 0
    dw 31
    dw 0x8000
    dw 0x0000
    dq 1

times 510-($-$$) db 0
dw 0xaa55
