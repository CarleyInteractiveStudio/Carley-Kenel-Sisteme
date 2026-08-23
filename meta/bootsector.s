.code16
.global _start
_start:
    cli
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %sp

    # Save Boot Drive Number passed by BIOS in DL
    mov %dl, (boot_drive)

    # Enable A20 Line
    in $0x92, %al
    or $2, %al
    out %al, $0x92

    # Load 64 sectors (32 KB) from disk starting at Sector 2 (LBA 1) into 0x7E00
    mov $0x0240, %ax     # ah = 0x02 (Read Sectors), al = 64 sectors
    mov $0x0002, %cx     # ch = cylinder 0, cl = sector 2
    xor %dh, %dh         # dh = head 0
    mov (boot_drive), %dl# dl = boot drive
    mov $0x7E00, %bx     # es:bx = buffer address 0x0000:0x7E00
    int $0x13

    # Load GDT
    lgdt gdt_descriptor

    # Set CR0 PE bit (Protected Mode Enable)
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

    # Far jump to 32-bit code segment (0x08)
    ljmp $0x08, $pm_entry

.code32
pm_entry:
    # Setup 32-bit Data Segments
    mov $0x10, %eax
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    mov %eax, %ss
    mov $0x90000, %esp   # Setup stack at 0x90000

    # Jump to Kernel Entry Point loaded at 0x7E00
    jmp 0x7E00

.code16
boot_drive: .byte 0

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
