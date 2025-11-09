.intel_syntax noprefix
.section .note.GNU-stack, "", @progbits

.section .text
.global InternalSyscallImpl
.align 16
// https://man7.org/linux/man-pages/man2/syscall.2.html
// CC Args: RDI, RSI, RDX, RCX, R8, R9
// CC Caller-saved: RAX, RCX, RDX, R8, R9, R10, R11
// Linux syscall CC Args: RDI, RSI, RDX, R10, R8, R9
// Syscall no: RAX
InternalSyscallImpl:
    mov rax, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r10, r8
    mov r8, r9
    mov r9, [rsp + 8]
    syscall
    ret
