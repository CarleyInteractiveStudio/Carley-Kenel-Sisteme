/* Linker Script para Carley Kernel x86_64 compatible con Limine */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);          /* R-X */
    rodata PT_LOAD FLAGS(4);        /* R-- */
    data PT_LOAD FLAGS(6);          /* RW- */
}

SECTIONS
{
    /* Limine recomienda esta dirección base para kernels de 64 bits */
    . = 0xffffffff80000000;

    .text : {
        *(.text*)
    } :text

    . = ALIGN(0x1000);

    .rodata : {
        *(.rodata*)
    } :rodata

    . = ALIGN(0x1000);

    .data : {
        /* Secciones obligatorias para Limine */
        KEEP(*(.requests_start_marker))
        KEEP(*(.requests))
        KEEP(*(.requests_end_marker))
        *(.data*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss*)
    } :data

    /DISCARD/ :
    {
        *(.interp)
        *(.note*)
        *(.comment)
    }
}
