[bits 16]
[org 0x7E00]

stage2_start:
    ; 1. Normalización total de registros
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    ; Trace: 'S' (Stage 2)
    mov ax, 0x0e53
    xor bx, bx
    int 0x10

    ; 2. Hardware init: A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Detectar Memoria E820
    mov di, 0x9000
    xor ebx, ebx
    mov edx, 0x534D4150
    mov dword [mem_count_extended], 0
.do_e820:
    mov eax, 0xe820
    mov ecx, 24
    int 0x15
    jc .e820_done
    add di, 24
    inc dword [mem_count_extended]
    test ebx, ebx
    jne .do_e820
.e820_done:

    ; Buscar RSDP (ACPI) en 0xE0000 - 0xFFFFF
    mov dword [rsdp_addr_low], 0
    mov ax, 0xE000
    mov es, ax
    xor di, di
.search_rsdp:
    cmp dword [es:di], 'RSD '
    jne .next_rsdp
    cmp dword [es:di+4], 'PTR '
    je .found_rsdp
.next_rsdp:
    add di, 16
    jnz .search_rsdp
    ; Probar segmento 0xF000 (BIOS usualmente pone RSDP aquí)
    mov ax, es
    cmp ax, 0xF000
    je .rsdp_done
    mov ax, 0xF000
    mov es, ax
    jmp .search_rsdp

    ; Si llegamos aquí y no lo encontramos, rsdp_addr_low queda en 0
    jmp .rsdp_done
.found_rsdp:
    xor eax, eax
    mov ax, es
    shl eax, 4
    movzx edx, di
    add eax, edx
    mov [rsdp_addr_low], eax
.rsdp_done:
    xor ax, ax
    mov es, ax

    ; Trace: 'E'
    mov ax, 0x0e45
    int 0x10

    ; 3. Detección REAL de Video (Evita Error Crítico en VBox)
    mov ax, 0x4f01
    mov cx, 0x4118 ; 1024x768x32
    mov di, 0x7000 ; ModeInfoBlock
    int 0x10

    ; 4. Cargar el Kernel desde el disco (LBA 256+)
    ; Buscamos en el sistema de archivos CarleyFS (LBA 128)
    mov dword [dap_lba], 128 ; Superbloque
    mov word [dap_count], 1
    mov word [dap_segment], 0x1000 ; Buffer 0x10000
    mov word [dap_offset], 0x0000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_s2

    ; Verificar Magic (usando GS para no contaminar DS)
    mov ax, 0x1000
    mov gs, ax
    cmp dword [gs:0], 0xCA121E1
    jne disk_error_s2

    ; Leer Inodos (LBA 132)
    mov dword [dap_lba], 132
    mov word [dap_count], 32
    mov word [dap_segment], 0x1020 ; Buffer 0x10200
    int 0x13
    jc disk_error_s2

    ; Buscar "kernel"
    ; PROTECCIÓN: Empujamos DS y ES
    push ds
    push es
    mov ax, 0x1020
    mov ds, ax     ; DS -> Inodos
    xor si, si
    mov ax, 0
    mov es, ax     ; ES -> kernel_name
    mov cx, 64
.search_loop:
    push cx
    mov di, kernel_name
    mov cx, 6
    repe cmpsb
    pop cx
    je .found_kernel
    add si, 80
    loop .search_loop
    pop es
    pop ds
    jmp disk_error_s2

.found_kernel:
    mov eax, [si + 64] ; size
    mov ebx, [si + 68] ; start sector
    pop es
    pop ds ; DS RESTAURADO A 0 (Seguridad total)
    mov gs, ax ; Limpiar GS con 0 (AX es 0 por pop ds)

    mov [kernel_sectors_left_bytes], eax
    mov [kernel_lba_current], ebx
    mov edi, 0x100000 ; Destino final (1MB)

    ; Unreal Mode para escribir en memoria extendida
    push ds
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:.unreal
[bits 32]
.unreal:
    mov bx, 0x10
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0x18:.unreal_done
[bits 16]
.unreal_done:
    pop ds
    xor ax, ax
    mov es, ax

.load_loop:
    mov eax, [kernel_lba_current]
    mov [dap_lba], eax
    mov word [dap_count], 64
    mov word [dap_segment], 0x4000 ; Buffer 0x40000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_s2

    ; Copiar a 1MB usando Unreal Mode (GS)
    mov ecx, (64 * 512) / 4
    mov esi, 0x40000
.copy:
    mov eax, [gs:esi]
    mov [gs:edi], eax
    add esi, 4
    add edi, 4
    loop .copy

    add dword [kernel_lba_current], 64
    cmp dword [kernel_sectors_left_bytes], (64 * 512)
    jbe .kernel_ok
    sub dword [kernel_sectors_left_bytes], (64 * 512)
    jmp .load_loop

.kernel_ok:
    ; Trace: 'K'
    mov ax, 0x0e4b
    xor bx, bx
    int 0x10

    ; 5. Activar Modo Gráfico
    mov ax, 0x4f02
    mov bx, 0x4118
    int 0x10

    ; 6. Salto a Modo Protegido
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm_start

disk_error_s2:
    push ax
    mov al, 'F'
    mov ah, 0x0e
    int 0x10
    pop ax
    mov al, ah ; Error code from int 13h is in AH
    call print_hex
    hlt

; Helper to print AL as hex
print_hex:
    pusha
    mov cx, 2
.loop:
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
    loop .loop
    popa
    ret

kernel_name db "kernel", 0

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; 7. Paginación de 64 bits (Identity + HHDM)
    mov edi, 0x20000
    mov cr3, edi
    xor eax, eax
    mov ecx, 6144 ; Limpiar 24KB (6 páginas para PML4, PDPT y 4 PDs)
    rep stosd

    mov dword [0x20000], 0x21003
    mov dword [0x20000 + 256*8], 0x21003
    mov dword [0x21000], 0x22003
    mov dword [0x21008], 0x23003
    mov dword [0x21010], 0x24003
    mov dword [0x21018], 0x25003

    mov edi, 0x22000
    mov eax, 0x00000083
    mov ecx, 2048
.map:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map

    ; 8. Long Mode
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt_ptr_long]
    jmp 0x08:long_mode_start

[bits 64]
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x9FFF0 ; Alineada a 16 bytes

    ; 9. Pasar Boot Info (Estructura boot_info_t packed)
    ; Offset 0: framebuffer_address (8 bytes)
    mov eax, [0x7000 + 40] ; PhysBasePtr real desde VBE Mode Info Block
    mov [0x6000], rax
    ; Offset 8: screen_width (4 bytes)
    mov dword [0x6008], 1024
    ; Offset 12: screen_height (4 bytes)
    mov dword [0x600c], 768
    ; Offset 16: memory_map_address (8 bytes)
    mov qword [0x6010], 0x9000
    ; Offset 24: memory_map_count (4 bytes)
    mov eax, [mem_count_extended]
    mov [0x6018], eax
    ; Offset 28: rsdp_address (8 bytes) - NOTA: Packed struct pone esto en 28
    mov eax, [rsdp_addr_low]
    mov [0x601C], rax

    mov rdi, 0x6000
    mov rax, 0xFFFF800000100000
    call rax
    hlt

; --- Datos ---
mem_count_extended dd 0
rsdp_addr_low dd 0
boot_drive db 0
kernel_sectors_left_bytes dd 0
kernel_lba_current dd 0

align 16
dap:
    db 0x10, 0
dap_count: dw 0
dap_offset: dw 0
dap_segment: dw 0
dap_lba: dq 0

gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00009A000000FFFF
    dq 0x000092000000FFFF
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt_start_long:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end_long:
gdt_ptr_long:
    dw gdt_end_long - gdt_start_long - 1
    dq gdt_start_long
