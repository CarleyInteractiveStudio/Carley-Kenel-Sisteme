.global gdt_flush

.section .text
gdt_flush:
    lgdt (%rdi)          # Cargar la nueva GDT (rdi es el primer argumento: puntero a GDTR)

    # Recargar los registros de segmento para datos (Data Segments)
    mov $0x10, %ax       # Offset del selector de datos kernel (GDT entry 2: 0x10)
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Truco de far return para cargar CS con el selector de código (0x08)
    push $0x10           # 1. Empujar SS (Selector de stack actual)
    push %rsp            # 2. Empujar RSP (Stack pointer actual)
    pushfq               # 3. Empujar RFLAGS (Flags actuales)
    push $0x08           # 4. Empujar CS (Selector de código kernel: GDT entry 1: 0x08)
    lea .reload_cs(%rip), %rax
    push %rax            # 5. Empujar RIP (Dirección de retorno)

    iretq                # Ejecutar inter-privilege level return (Carga CS y RIP)

.reload_cs:
    ret
