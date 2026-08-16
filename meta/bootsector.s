.code16
.global _start
_start:
    cli
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %sp

    # Load GDT
    lgdt gdt_descriptor

    # Set CR0 PE bit (Protected Mode Enable)
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

    # Far jump to 32-bit code segment (0x08)
    ljmp $0x08, $0x7E00

.align 4
gdt_start:
    .quad 0x0000000000000000 # Null descriptor
gdt_code: # Code segment: base 0x0, limit 4GB, 32-bit code
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x9A, 0xCF, 0x00
gdt_data: # Data segment: base 0x0, limit 4GB, 32-bit data
    .word 0xFFFF, 0x0000
    .byte 0x00, 0x92, 0xCF, 0x00
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start

.fill 510 - (. - _start), 1, 0
.word 0xAA55
