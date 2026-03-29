[bits 16]
[org 0x8000]

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov [boot_drive], dl    ; Guardar unidad de arranque

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

    ; 2.5 Cargar el Kernel desde el disco a 1MB
    ; Usaremos "Unreal Mode" para copiar datos por encima de 1MB en modo real
    push ds
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp next_step
next_step:
    mov bx, 0x10 ; Selector de datos
    mov ds, bx
    mov es, bx
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    pop ds
    ; Ahora estamos en Unreal Mode.

    ; Cargar el Kernel. Como int 0x13 no puede cargar por encima de 1MB,
    ; cargamos en 0x2000:0x0000 (128KB) en trozos y movemos a 1MB (0x100000).
    mov dword [kernel_sectors_left], 2048 ; 2048 sectores = 1MB (tamaño máximo kernel aprox)
    mov dword [kernel_lba_current], 2048 ; LBA inicial del Kernel
    mov edi, 0x100000 ; Destino final

load_kernel_loop:
    mov eax, [kernel_lba_current]
    mov [dap_lba], eax
    mov word [dap_count], 64
    mov word [dap_segment], 0x2000

    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_stage2

    ; Copiar a memoria extendida usando modo protegido temporalmente (más seguro que unreal mode)
    cli
    push ds
    push es
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_copy_kernel

[bits 32]
pm_copy_kernel:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov esi, 0x20000
    ; Usamos EBP para guardar EDI a traves del cambio de modo
    mov ecx, (64 * 512) / 4
    rep movsd
    mov ebp, edi

    ; Volver a modo real
    ; Antes de volver, cargar selectores de 64KB para evitar problemas
    mov ax, 0x18
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0x00:rm_copy_kernel

[bits 16]
rm_copy_kernel:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov edi, ebp
    pop es
    pop ds
    sti

    add dword [kernel_lba_current], 64
    sub dword [kernel_sectors_left], 64
    jnz load_kernel_loop

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

disk_error_stage2:
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
    ;    Mapeamos el primer Gigabyte (Identity Mapping)
    ;    Usamos 0x20000 como buffer seguro para las tablas (ya movimos el kernel de ahi)
    mov edi, 0x20000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    rep stosd

    mov dword [0x20000], 0x21003 ; PML4[0] -> PDPT (0x21000)
    mov dword [0x21000], 0x22003 ; PDPT[0] -> PD (0x22000)
    mov edi, 0x22000
    mov eax, 0x00000083        ; 2MB Pages | Writable | Present
    mov ecx, 512
map_loop:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop map_loop

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

    mov rax, [0x7000 + 40]    ; LFB Address de VBE Info
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
boot_drive db 0
kernel_sectors_left dd 0
kernel_lba_current dd 0

; Disk Address Packet (DAP) para int 13h ah=42h
align 4
dap:
    db 0x10          ; Tamaño del DAP
    db 0             ; Reservado
dap_count:
    dw 0             ; Numero de sectores a leer
    dw 0x0000        ; Offset del buffer
dap_segment:
    dw 0x0000        ; Segmento del buffer
dap_lba:
    dq 0             ; LBA inicial

; Nota: Como cargar directamente a 1MB con int 13h es problematico en algunos BIOS,
; cargaremos en 0x10000 (64KB) y luego moveremos a 1MB usando Unreal Mode.
; Pero para simplificar y dado que el Kernel puede ser grande, usaremos una direccion segura.
; Vamos a re-estructurar el DAP para cargar en 0x10000 y luego moverlo.

; GDTs
gdt_start:
    dq 0, 0x00cf9a000000ffff, 0x00cf92000000ffff, 0x000092000000ffff
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
