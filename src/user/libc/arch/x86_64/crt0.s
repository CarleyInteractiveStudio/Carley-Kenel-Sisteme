.section .text
.global _start
.type _start, @function
.extern main

_start:
    xor %rbp, %rbp

    # En el ELF loader pusheamos argc a RDI y argv a RSI
    # Llamamos directamente a main(rdi, rsi)
    call main

    # SYS_EXIT con el retorno de main (RAX)
    mov %rax, %rdi
    mov $8, %rax
    int $0x80

    1: jmp 1b
.size _start, .-_start

.section .note.GNU-stack,"",@progbits
