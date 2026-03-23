.section .text
.global _start
.extern main

_start:
    # Preparar el stack frame inicial
    xor %rbp, %rbp

    # Llamar a la función main del programa de usuario
    # Los argumentos (argc, argv) se manejarán en el futuro
    call main

    # Syscall de salida (por ahora usamos yield como terminación suave)
    mov $0, %rax # SYS_YIELD
    int $0x80

    # Bucle infinito por si acaso
    1: jmp 1b
