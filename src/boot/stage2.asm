[bits 16]
[org 0x8000]

stage2_start:
    cli
    mov [boot_drive_saved], dl ; Guardar el disco de arranque
    xor ax, ax
    mov ds, ax
    mov es, ax

    ; 1. Habilitar la línea A20 (para acceder a toda la memoria)
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 2. Detectar Mapa de Memoria BIOS (E820)
    ;    Guardaremos el mapa en 0x9000
    mov di, 0x9000
    xor ebx, ebx
    mov edx, 0x534D4150    ; 'SMAP'
    mov word [mem_count], 0
do_e820:
    mov eax, 0xe820
    mov ecx, 24
    int 0x15
    jc e820_done
    cmp eax, 0x534D4150
    jne e820_done
    add di, 24
    inc word [mem_count]
    test ebx, ebx
    jne do_e820
e820_done:

    ; 2.5 Cargar el Kernel (LBA 2048) a 1MB usando Unreal Mode
    push ds
    lgdt [unreal_gdt_ptr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp $+2
    mov bx, 0x08
    mov ds, bx
    mov es, bx
    and al, 0xfe
    mov cr0, eax
    pop ds

    ; Cargar 1024 sectores (512KB) del Kernel
    mov edi, 0x100000 ; Destino: 1MB
    mov ebx, 2048     ; Sector inicial en el disco
    mov ecx, 1024     ; Cantidad de sectores

load_kernel_loop:
    push ecx
    push edi
    mov [dap_lba], ebx
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive_saved]
    int 0x13
    jc disk_error_halt

    ; Copiar del buffer temporal (0x0000:0x1000) al destino final (EDI)
    mov esi, 0x1000   ; Buffer temporal
    mov ecx, 128      ; 512 bytes / 4
    db 0x67           ; Prefijo para usar EDI de 32 bits
    rep movsd

    pop edi
    add edi, 512
    pop ecx
    inc ebx
    loop load_kernel_loop

    ; 3. Configurar Modo de Video VBE (1024x768x32)
    ;    Obtener información del modo 0x118 (1024x768x32)
    mov ax, 0x4f01
    mov cx, 0x118          ; Modo 1024x768x32
    mov di, 0x7000         ; Buffer para VBE Mode Info
    int 0x10
    cmp ax, 0x004f
    jne video_error

    ; Activar el modo de video
    mov ax, 0x4f02
    mov bx, 0x4118         ; Modo 0x118 + bit 14 (Linear Framebuffer)
    int 0x10
    cmp ax, 0x004f
    jne video_error

    ; 4. Preparar Paso a 32-bit (Modo Protegido)
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_start

video_error:
    mov ah, 0x0e
    mov al, 'V'
    int 0x10
    hlt

disk_error_halt:
    mov ah, 0x0e
    mov al, 'D'
    int 0x10
    hlt

[bits 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; 5. Preparar Paginación para Modo Largo (64-bit)
    ;    Mapeamos los primeros 4 Gigabytes (Identity Mapping) para cubrir el Framebuffer
    mov edi, 0x10000
    mov cr3, edi
    xor eax, eax
    mov ecx, 8192 ; Limpiar 8 páginas (PML4, PDPT y 4 PDs)
    rep stosd

    mov dword [0x10000], 0x11003 ; PML4[0] -> PDPT
    mov dword [0x11000], 0x12003 ; PDPT[0] -> PD0
    mov dword [0x11008], 0x13003 ; PDPT[1] -> PD1
    mov dword [0x11010], 0x14003 ; PDPT[2] -> PD2
    mov dword [0x11018], 0x15003 ; PDPT[3] -> PD3

    mov edi, 0x12000 ; Inicio de PD0
    mov eax, 0x00000083 ; 2MB Pages | Writable | Present
    mov ecx, 2048 ; 512 * 4 = 2048 entradas (4GB)
.map_loop_4gb:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_loop_4gb

    ; 6. Habilitar PAE y Long Mode
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

    ; 7. Preparar la estructura Boot Info para el Kernel
    ;    La pondremos en 0x6000
    ;    [0x6000]: Framebuffer Address (8 bytes)
    ;    [0x6008]: Screen Width (4 bytes)
    ;    [0x600c]: Screen Height (4 bytes)
    ;    [0x6010]: Memory Map Address (8 bytes)
    ;    [0x6018]: Memory Map Count (4 bytes)

    mov eax, [0x7000 + 40]    ; LFB Address de VBE Info (Offset 40 en struct vbe_mode_info)
    mov [0x6000], rax
    mov dword [0x6008], 1024
    mov dword [0x600c], 768
    mov qword [0x6010], 0x9000
    movzx rax, word [mem_count]
    mov [0x6018], eax

    ; Pasar el puntero de Boot Info en RDI (primer argumento de C)
    mov rdi, 0x6000

    ; Saltar al kernel en 1MB
    mov rax, 0x100000
    jmp rax

mem_count dw 0
boot_drive_saved db 0

; Estructura para lectura LBA (DAP)
dap:
    db 0x10
    db 0
    dw 1      ; Leer 1 sector a la vez
    dw 0x1000 ; Buffer temporal (Offset)
    dw 0x0000 ; Buffer temporal (Segmento)
dap_lba:
    dq 0      ; LBA se llena en el bucle

; GDT para Modo Unreal
unreal_gdt:
    dq 0
    dq 0x00cf92000000ffff ; Data segment (limit 4GB)
unreal_gdt_ptr:
    dw 15
    dd unreal_gdt

; GDTs
gdt_start:
    dq 0, 0x00cf9a000000ffff, 0x00cf92000000ffff
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt_start_long:
    dq 0, 0x00209a0000000000, 0x0000920000000000
gdt_end_long:
gdt_ptr_long:
    dw gdt_end_long - gdt_start_long - 1
    dq gdt_start_long
