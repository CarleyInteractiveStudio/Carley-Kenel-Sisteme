.extern irq_handler

.macro ISR_NOERR num
.global isr\num
isr\num:
    cli
    push $0
    push $\num
    jmp isr_common_stub
.endm

.macro ISR_ERR num
.global isr\num
isr\num:
    cli
    push $\num
    jmp isr_common_stub
.endm

.macro IRQ num, map
.global irq\num
irq\num:
    cli
    push $0
    push $\map
    jmp isr_common_stub
.endm

# Excepciones
ISR_NOERR 0; ISR_NOERR 1; ISR_NOERR 2; ISR_NOERR 3; ISR_NOERR 4; ISR_NOERR 5; ISR_NOERR 6; ISR_NOERR 7
ISR_ERR 8; ISR_ERR 10; ISR_ERR 11; ISR_ERR 12; ISR_ERR 13; ISR_ERR 14
ISR_NOERR 15; ISR_NOERR 16; ISR_ERR 17; ISR_NOERR 18; ISR_NOERR 19; ISR_NOERR 20; ISR_ERR 30

# IRQs
IRQ 0, 32
IRQ 1, 33
IRQ 12, 44

# Syscalls
.global isr128
isr128:
    cli
    push $0
    push $128
    jmp isr_common_stub

isr_common_stub:
    # 1. Guardar todos los registros generales
    push %rax; push %rbx; push %rcx; push %rdx; push %rdi; push %rsi; push %rbp; push %r8
    push %r9; push %r10; push %r11; push %r12; push %r13; push %r14; push %r15

    # 2. Pasar puntero al contexto actual (RSP) como argumento a irq_handler
    mov 15*8(%rsp), %rdi      # int_no
    mov %rsp, %rsi            # context_ptr

    call irq_handler

    # 3. Mover RSP al nuevo contexto devuelto por el scheduler
    mov %rax, %rsp

    # 4. Restaurar registros
    pop %r15; pop %r14; pop %r13; pop %r12; pop %r11; pop %r10; pop %r9; pop %r8
    pop %rbp; pop %rsi; pop %rdi; pop %rdx; pop %rcx; pop %rbx; pop %rax

    # 5. Limpiar stack de int_no y error_code
    add $16, %rsp

    iretq

.section .note.GNU-stack,"",@progbits
