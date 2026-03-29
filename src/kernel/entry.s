.intel_syntax noprefix
.global _start
.extern kmain

_start:
    jmp kmain
    .long 0xC0DEB007
