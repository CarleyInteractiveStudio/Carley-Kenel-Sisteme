.global gdt_flush

.section .text
gdt_flush:
    lgdt (%rdi)          # Cargar la nueva GDT

    # Recargar los registros de segmento para datos
    mov $0x10, %ax       # Offset del selector de datos kernel
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Realizar un 'far return' usando lretq para recargar CS limpiamente
    # Primero empujamos el selector de código y luego la dirección de retorno
    push $0x08           # Nuevo CS (Kernel Code)
    lea .reload_cs(%rip), %rax
    push %rax            # Dirección de retorno
    lretq

.reload_cs:
    ret

.section .note.GNU-stack,"",@progbits
