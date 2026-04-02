[bits 16]
[org 0x7c00]

; --- CARLEY OS MBR v14 (ULTRA RECOVERY) ---

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

    ; 'B' (MBR)
    mov al, 'B'
    call debug_char

    ; 1. Check if Stage 2 is ALREADY in memory (BIOS might have loaded it)
    cmp dword [0x7E00], 0xDEADBEEF
    je jump_now
    cmp dword [0x8000], 0xDEADBEEF ; Check at +512 (Standard HDD load)
    je jump_now_8000

    ; 2. Try to load from Disk using LBA candidates
    ; Candidate 1: LBA 1 (CD-ROM standard)
    mov dword [dap_lba], 1
    call try_load
    jc .fail_1
    cmp dword [0x7E00], 0xDEADBEEF
    je jump_now
.fail_1:
    mov al, '1'
    call debug_char

    ; Candidate 4: LBA 4 (HDD standard with 2048B padding)
    mov dword [dap_lba], 4
    call try_load
    jc .fail_4
    cmp dword [0x7E00], 0xDEADBEEF
    je jump_now
.fail_4:
    mov al, '4'
    call debug_char

    ; 3. Fallback to Floppy/Legacy Mode (CHS? No, stick to LBA 0 + 1)
    ; Maybe it's at LBA 0 (after MBR)?
    mov dword [dap_lba], 0
    call try_load
    cmp dword [0x7E00 + 512], 0xDEADBEEF ; Check if signature is at offset 512 of what we loaded
    je jump_now_512

    ; 4. Final Error - Print Error Code and Hang
    mov al, 'E'
    call debug_char
    mov ah, 0x01 ; Get status of last drive operation
    mov dl, [boot_drive]
    int 0x13
    mov al, ah
    call debug_hex
    jmp hang

jump_now_8000:
    ; Relocate 0x8000 to 0x7E00 for consistency
    mov si, 0x8000
    mov di, 0x7E00
    mov cx, 16384
    rep movsb
    jmp jump_now

jump_now_512:
    ; Relocate 0x7E00 + 512 to 0x7E00
    mov si, 0x7E00 + 512
    mov di, 0x7E00
    mov cx, 16384
    rep movsb
    jmp jump_now

jump_now:
    mov al, 'V'
    call debug_char
    mov dl, [boot_drive]
    jmp 0:0x7E04 ; Skip signature

try_load:
    ; Reset disk
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13
    ; Load
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    ret

debug_char:
    mov ah, 0x0e
    int 0x10
    ret

debug_hex:
    push ax
    shr al, 4
    call .hex_digit
    pop ax
    and al, 0x0F
    call .hex_digit
    ret
.hex_digit:
    add al, '0'
    cmp al, '9'
    jbe .print
    add al, 7
.print:
    mov ah, 0x0e
    int 0x10
    ret

hang:
    hlt
    jmp hang

boot_drive db 0
align 4
dap:
    db 0x10, 0
    dw 64         ; Load 32KB
    dw 0x7E00, 0
dap_lba:
    dq 1

times 510-($-$$) db 0
dw 0xaa55
