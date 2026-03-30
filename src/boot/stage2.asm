[bits 16]
[org 0x8000]

stage2_start:
    ; Normalizar registros inmediatamente tras el salto
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Trace: '!' (Salto exitoso)
    mov ax, 0x0e21
    xor bx, bx
    int 0x10

    jmp actual_start
    nop

actual_start:
    ; 1. Habilitar la línea A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 2. Detectar Mapa de Memoria BIOS (E820)
    mov di, 0x9000
    xor ebx, ebx
    mov edx, 0x534D4150
    mov dword [mem_count_extended], 0
.do_e820:
    mov eax, 0xe820
    mov ecx, 24
    int 0x15
    jc .e820_done
    cmp eax, 0x534D4150
    jne .e820_done
    add di, 24
    inc dword [mem_count_extended]
    cmp dword [mem_count_extended], 128
    jae .e820_done
    test ebx, ebx
    jne .do_e820
.e820_done:
    mov al, 'E'
    mov ah, 0x0e
    int 0x10

    ; 3. Verificación de Hardware (Long Mode)
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_long_mode

    mov al, 'C'
    int 0x10

    ; 4. Cargar el Kernel desde CarleyFS
    ; Leer Superbloque (LBA 64)
    mov dword [dap_lba], 64
    mov word [dap_count], 1
    mov word [dap_segment], 0x1000 ; Buffer en 0x10000
    mov word [dap_offset], 0x0000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_s2

    mov ax, 0x1000
    mov gs, ax
    cmp dword [gs:0], 0xCA121E1
    jne disk_error_s2

    ; Leer Tabla de Inodos (LBA 65)
    mov dword [dap_lba], 65
    mov word [dap_count], 32
    mov word [dap_segment], 0x1020
    int 0x13
    jc disk_error_s2

    ; Buscar "kernel"
    mov ax, 0x1020
    mov ds, ax
    xor si, si
    mov cx, 64
.search_kernel:
    push cx
    mov di, kernel_name
    xor bx, bx
    mov es, bx ; ES:DI -> kernel_name
    mov cx, 6
    push si
    repe cmpsb
    pop si
    pop cx
    je .found_kernel
    add si, 80
    loop .search_kernel
    jmp disk_error_s2

.found_kernel:
    mov eax, [si + 64] ; size
    xor bx, bx
    mov es, bx
    mov [es:kernel_sectors_left_bytes], eax
    mov eax, [si + 68] ; start
    mov [es:kernel_lba_current], eax
    mov edi, 0x100000

    ; Entrar en Unreal Mode para cargar a 1MB
    push ds
    xor ax, ax
    mov ds, ax
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:unreal_setup
[bits 32]
unreal_setup:
    mov bx, 0x10
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0x18:unreal_done
[bits 16]
unreal_done:
    xor ax, ax
    mov ds, ax
    mov es, ax

.load_loop:
    mov eax, [kernel_lba_current]
    mov [dap_lba], eax
    mov word [dap_count], 64
    mov word [dap_segment], 0x4000 ; 0x40000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_s2

    mov ecx, (64 * 512) / 4
    mov esi, 0x40000
.inner_copy:
    mov eax, [gs:esi]
    mov [gs:edi], eax
    add esi, 4
    add edi, 4
    loop .inner_copy

    add dword [kernel_lba_current], 64
    cmp dword [kernel_sectors_left_bytes], (64 * 512)
    jbe .kernel_loaded
    sub dword [kernel_sectors_left_bytes], (64 * 512)
    jmp .load_loop

.kernel_loaded:
    mov al, 'K'
    mov ah, 0x0e
    int 0x10

    ; 5. Obtener información VBE real (Importante para evitar crash)
    mov ax, 0x4f01
    mov cx, 0x118
    mov di, 0x7000 ; Buffer para ModeInfo
    int 0x10

    ; 6. Configurar Modo de Video
    mov ax, 0x4f02
    mov bx, 0x4118
    int 0x10

    ; 7. Paso a Modo Protegido
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_start

no_long_mode:
    mov si, msg_err_cpu
    call print_string
    jmp $

disk_error_s2:
    mov si, msg_err_disk
    call print_string
    hlt

clear_screen:
    mov ax, 0x03
    int 0x10
    ret

print_string:
    mov ah, 0x0e
    xor bx, bx
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

msg_err_cpu db "ERROR: No 64-bit.", 0
msg_err_disk db "ERROR: No Kernel.", 0
kernel_name db "kernel", 0

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; 8. Paginación (Higher Half)
    mov edi, 0x20000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
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
.map_loop:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_loop

    ; 9. Entrar en Long Mode
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
    mov rsp, 0x9FFF0 ; Alineado a 16 bytes para C ABI

    ; Boot Info en 0x6000
    mov eax, [0x7000 + 40] ; PhysBasePtr (Dirección REAL del framebuffer)
    mov [0x6000], rax
    mov dword [0x6008], 1024
    mov dword [0x600c], 768
    mov qword [0x6010], 0x9000 ; Mem Map
    mov eax, [mem_count_extended]
    mov [0x6018], eax

    mov rdi, 0x6000
    mov rax, 0xFFFF800000100000
    call rax
    jmp $

mem_count_extended dd 0
boot_drive db 0
kernel_sectors_left_bytes dd 0
kernel_lba_current dd 0

align 4
dap:
    db 0x10
    db 0
dap_count:
    dw 0
dap_offset:
    dw 0x0000
dap_segment:
    dw 0x0000
dap_lba:
    dq 0

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
