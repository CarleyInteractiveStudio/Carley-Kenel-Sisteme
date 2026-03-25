.section .text
.global _start
.extern ld_main

_start:
    # El kernel nos pasa:
    # RDI = argc
    # RSI = argv
    # RDX = original entry point (app_entry)

    # Preparamos los argumentos para ld_main(argc, argv, app_entry)
    # Ya están en RDI, RSI, RDX. Simplemente llamamos.

    call ld_main

    # Si ld_main retorna (que no debería), salimos.
    mov $0, %rdi
    mov $8, %rax
    int $0x80

    1: jmp 1b
