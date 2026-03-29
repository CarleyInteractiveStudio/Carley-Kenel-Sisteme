/* Linker Script para Carley Kernel x86_64 */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(kmain)

SECTIONS
{
    /* Ubicación física (1MB) y virtual del kernel (Higher Half 64-bit canonical) */
    . = 0xFFFF800000100000;

    .text : AT(0x100000)
    {
        *(.text*)
    } :text

    .rodata :
    {
        *(.rodata*)
    } :rodata

    .data :
    {
        *(.requests) /* Seccion obligatoria para el protocolo Limine */
        *(.data*)
    } :data

    .bss :
    {
        *(COMMON)
        *(.bss*)
    } :bss

    /DISCARD/ :
    {
        *(.interp)
        *(.note*)
        *(.comment)
    }
}

PHDRS
{
    text PT_LOAD FLAGS(5);          /* R-X */
    rodata PT_LOAD FLAGS(4);        /* R-- */
    data PT_LOAD FLAGS(6);          /* RW- */
    bss PT_LOAD FLAGS(6);           /* RW- */
}
