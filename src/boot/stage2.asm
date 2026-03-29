[bits 16]
[org 0x7E00]

stage2_start:
    ; Señal visual: Pantalla ROJA (Fase 1: Inicio Stage 2)
    mov ax, 0x0700
    mov bh, 0x4F
    xor cx, cx
    mov dx, 0x184F
    int 0x10

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
    mov dword [mem_count_extended], 0
do_e820:
    mov eax, 0xe820
    mov ecx, 24
    int 0x15
    jc e820_done
    cmp eax, 0x534D4150
    jne e820_done
    add di, 24
    inc dword [mem_count_extended]
    test ebx, ebx
    jne do_e820
e820_done:

    ; Imprimir 'S' (Stage 2 started)
    mov ah, 0x0e
    mov al, 'S'
    int 0x10

    ; 2.5 Cargar el Kernel desde el disco a 1MB
    ; Detectar si estamos en CD (sector size 2048) o HDD (sector size 512)
    ; Intentamos primero LBA 2048 (Disco Duro)
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
    mov dword [kernel_sectors_left], 4096
    mov dword [kernel_lba_current], 2048
    mov edi, 0x100000

load_kernel_loop:
    ; Verificar si el primer sector que vamos a cargar tiene la firma mágica
    cmp dword [kernel_sectors_left], 4096
    jne perform_load

    ; Intentar encontrar el kernel en LBA 2048 (HDD)
    mov dword [dap_lba], 2048
    mov word [dap_count], 1
    mov word [dap_segment], 0x2000
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13

    ; Usar registro para evitar advertencias de tamaño en 16 bits
    ; Buscamos en el offset 5 (el salto 'jmp kmain' ocupa 5 bytes)
    mov ebx, 0x20005
    mov eax, [ebx]
    cmp eax, 0xC0DEB007
    je found_hdd

    ; Intentar encontrar el kernel en LBA 512 (CD-ROM)
    mov dword [dap_lba], 512
    mov si, dap
    mov ah, 0x42
    int 0x13

    mov ebx, 0x20005
    mov eax, [ebx]
    cmp eax, 0xC0DEB007
    je found_cd

    ; Si no se encuentra, error fatal
    jmp disk_error_stage2

found_hdd:
    mov dword [kernel_lba_current], 2048
    jmp perform_load
found_cd:
    mov dword [kernel_lba_current], 512

perform_load:
    mov eax, [kernel_lba_current]
    mov [dap_lba], eax
    mov word [dap_count], 64
    mov word [dap_segment], 0x2000

    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error_stage2

    ; Señal visual: Pantalla VERDE (Fase 2: Cargando Kernel)
    mov ax, 0x0700
    mov bh, 0x2F
    xor cx, cx
    mov dx, 0x184F
    int 0x10

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
    ; Antes de volver, cargar selectores de 16-bit
    jmp 0x18:pm_to_rm
[bits 16]
pm_to_rm:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    jmp 0x0000:rm_copy_kernel

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

    ; Señal visual: Pantalla CYAN (Fase 3: Kernel en memoria, activando VBE)
    mov ax, 0x0700
    mov bh, 0x3F
    xor cx, cx
    mov dx, 0x184F
    int 0x10

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

    ; PML4[0] -> PDPT (0x21000)
    mov dword [0x20000], 0x21003
    mov dword [0x20000 + 4], 0

    ; PDPT[0,1,2,3] -> PDs (0x22000, 0x23000, 0x24000, 0x25000) para mapear 4GB
    mov dword [0x21000], 0x22003
    mov dword [0x21000 + 4], 0
    mov dword [0x21008], 0x23003
    mov dword [0x21008 + 4], 0
    mov dword [0x21010], 0x24003
    mov dword [0x21010 + 4], 0
    mov dword [0x21018], 0x25003
    mov dword [0x21018 + 4], 0

    ; Rellenar las 4 tablas de directorio de páginas (512 entradas de 2MB cada una)
    mov edi, 0x22000
    mov eax, 0x00000083        ; Base 0x0, 2MB Pages, Writable, Present
    mov ecx, 2048              ; 512 * 4 = 2048 entradas (4GB totales)
map_4gb_loop:
    mov [edi], eax
    mov dword [edi + 4], 0     ; Asegurar bits altos en 0
    add eax, 0x200000          ; Siguiente página de 2MB
    add edi, 8
    loop map_4gb_loop

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
    mov fs, ax
    mov gs, ax
    mov rsp, 0x9FFFF ; Stack al final de los primeros 640KB

    ; 7. Preparar la estructura Boot Info para el Kernel
    ;    La pondremos en 0x6000
    ;    [0x6000]: Framebuffer Address (8 bytes)
    ;    [0x6008]: Screen Width (4 bytes)
    ;    [0x600c]: Screen Height (4 bytes)
    ;    [0x6010]: Memory Map Address (8 bytes)
    ;    [0x6018]: Memory Map Count (4 bytes)

    xor rax, rax
    mov eax, [0x7000 + 40]    ; LFB Address (32-bit)
    mov [0x6000], rax
    mov dword [0x6008], 1024
    mov dword [0x600c], 768
    mov qword [0x6010], 0x9000
    mov eax, [mem_count_extended]
    mov [0x6018], eax

    ; Pasar el puntero de Boot Info en RDI (primer argumento de C)
    mov rdi, 0x6000

    ; Imprimir 'K' (Kernel entry)
    mov rax, 0x0F4B0F4B0F4B0F4B
    mov [0xB8000], rax

    ; Saltar al kernel en 1MB
    mov rax, 0x100000
    call rax

    ; Si vuelve, detenerse
    jmp $

mem_count_extended dd 0
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
    dq 0                         ; Null
    dq 0x00CF9A000000FFFF       ; Code 32 (0x08)
    dq 0x00CF92000000FFFF       ; Data 32 (0x10)
    dq 0x00009A000000FFFF       ; Code 16 (0x18)
    dq 0x000092000000FFFF       ; Data 16 (0x20)
gdt_end:
gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt_start_long:
    dq 0                         ; Null
    dq 0x00AF9A000000FFFF       ; Code 64 (L bit set)
    dq 0x00CF92000000FFFF       ; Data 64
gdt_end_long:
gdt_ptr_long:
    dw gdt_end_long - gdt_start_long - 1
    dq gdt_start_long
