[bits 16]
[org 0x7E00]

; --- CARLEY STAGE 2 v14 (FIXED PAGING) ---
; Esta sección DEBE estar en 0x7E00.

    dd 0xDEADBEEF ; Firma mágica detectada por el MBR

stage2_entry:
    ; Marcador: Stage 2 Vivo
    mov al, 'S'
    mov ah, 0x0e
    int 0x10

    ; Forzar segmentación limpia
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00 ; Stack seguro debajo del MBR

    mov [boot_drive], dl

    ; Trace: Bienvenida
    mov si, msg_welcome
    call print_string

    ; 1. Hardware: A20 (Puerta a la memoria alta)
    call enable_a20
    mov al, 'A'
    call print_char

    ; 2. Unreal Mode (Acceso a 4GB desde 16-bit)
    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x18:.pm_temp

.pm_temp:
    mov ax, 0x10 ; Descriptor de datos 32-bit (flat)
    mov fs, ax
    mov gs, ax
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0:.real_temp

.real_temp:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov al, 'U'
    call print_char

    ; 3. Mapa de Memoria E820
    mov di, 0x9000
    xor ebx, ebx
    mov dword [mem_count_extended], 0
.do_e820:
    mov eax, 0xe820
    mov ecx, 24
    mov edx, 0x534D4150
    int 0x15
    jc .e820_done
    add di, 24
    inc dword [mem_count_extended]
    test ebx, ebx
    jne .do_e820
.e820_done:
    mov al, 'M'
    call print_char

    ; 4. Video VBE (1024x768x32)
    mov ax, 0x4f01
    mov cx, 0x4118
    mov di, 0x7000
    int 0x10
    mov al, 'G'
    call print_char

    ; 5. Cargar el Kernel
    ; El kernel está en el sector DATA_SECTOR_START (256)
    mov edi, 0x100000
    mov dword [kernel_lba_current], 256

.load_kernel_loop:
    mov eax, [kernel_lba_current]
    mov [dap_lba_s2], eax
    mov word [dap_count_s2], 4
    call read_sectors_s2
    call copy_to_high_mem_4segs

    add edi, 2048
    add dword [kernel_lba_current], 4

    ; Cargamos 1MB para el Kernel + Initrd
    cmp edi, 0x200000
    jl .load_kernel_loop

    mov al, 'K'
    call print_char

    ; 6. Salto Final al Kernel (64-bit)
    mov al, '!'
    call print_char

    ; Activar modo gráfico
    mov ax, 0x4f02
    mov bx, 0x4118
    int 0x10

    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm_start

; --- FUNCIONES ---
read_sectors_s2:
    pusha
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap_size_s2
    int 0x13
    popa
    ret

copy_to_high_mem_4segs:
    pusha
    mov esi, 0x1000 ; Bounce buffer
    mov ecx, 512    ; 2048 / 4 bytes
.c: a32 lodsd
    db 0x64, 0x67, 0x89, 0x07 ; mov [fs:edi], eax
    a32 add edi, 4
    loop .c
    popa
    ret

print_string:
    mov ah, 0x0e
.l: lodsb
    test al, al
    jz .d
    int 0x10
    jmp .l
.d: ret

print_char:
    mov ah, 0x0e
    int 0x10
    ret

enable_a20:
    mov ax, 0x2401
    int 0x15
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

; --- DATOS ---
msg_welcome db "CARLEY BOOT v14", 13, 10, 0
boot_drive db 0
mem_count_extended dd 0
kernel_lba_current dd 0

align 16
dap_size_s2 db 0x10, 0
dap_count_s2 dw 4
dap_off_s2  dw 0x1000
dap_seg_s2  dw 0x0000
dap_lba_s2  dq 0

gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF ; Code 32
    dq 0x00CF92000000FFFF ; Data 32
    dq 0x00009A000000FFFF ; Code 16
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; Configuración de Paginación (PML4, PDPT, PD)
    mov edi, 0x20000 ; PML4
    mov cr3, edi
    xor eax, eax
    mov ecx, 6144 ; 3 tablas de 4KB (PML4, PDPT, PD)
    rep stosd

    ; PML4 [0] = PDPT (Identity map)
    ; PML4 [256] = PDPT (HHDM: 0xFFFF8000...)
    mov dword [0x20000], 0x21003
    mov dword [0x20000 + 256*8], 0x21003

    ; PDPT [0] = PD
    mov dword [0x21000], 0x22003

    ; PD: Map 1GB with Huge Pages (2MB each)
    mov edi, 0x22000
    mov eax, 0x00000083 ; Present + Write + Huge
    mov ecx, 512        ; Exactamente una tabla (1GB)
.m: mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .m

    mov eax, cr4
    or eax, 1 << 5 ; PAE
    mov cr4, eax

    mov ecx, 0xc0000080 ; EFER
    rdmsr
    or eax, 1 << 8 ; LME
    wrmsr

    mov eax, cr0
    or eax, 1 << 31 ; Paging
    mov cr0, eax

    lgdt [gdt_ptr_long]
    jmp 0x08:long_mode_start

[bits 64]
long_mode_start:
    mov rsp, 0x9FFF0
    mov rdi, 0x6000
    mov rax, 0xFFFF800000100000
    call rax
    hlt

gdt_start_long:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end_long:
gdt_ptr_long:
    dw gdt_end_long - gdt_start_long - 1
    dq gdt_start_long
