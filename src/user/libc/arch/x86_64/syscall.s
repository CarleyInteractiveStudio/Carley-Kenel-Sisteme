.section .text

.global syscall0
.global syscall1
.global syscall2
.global syscall3
.global syscall4

# syscallX(num, arg1, arg2, ...)
# RAX = num, RDI = arg1, RSI = arg2, RDX = arg3, R10 = arg4

syscall0:
    mov %rdi, %rax
    int $0x80
    ret

syscall1:
    mov %rdi, %rax
    mov %rsi, %rdi
    int $0x80
    ret

syscall2:
    mov %rdi, %rax
    mov %rsi, %rdi
    mov %rdx, %rsi
    int $0x80
    ret

syscall3:
    mov %rdi, %rax
    mov %rsi, %rdi
    mov %rdx, %rsi
    mov %rcx, %rdx
    int $0x80
    ret

syscall4:
    # num=RDI, arg1=RSI, arg2=RDX, arg3=RCX, arg4=R8
    mov %rdi, %rax
    mov %rsi, %rdi
    mov %rdx, %rsi
    mov %rcx, %rdx
    mov %r8, %r10
    int $0x80
    ret

.section .note.GNU-stack,"",@progbits
