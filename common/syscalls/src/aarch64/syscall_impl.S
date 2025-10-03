.section .note.GNU-stack, "", @progbits

.section .text
.global InternalSyscallImpl
.align 16
// https://man7.org/linux/man-pages/man2/syscall.2.html
// CC Args: x0, x1, x2, x3, x4, x5, x6, x7
// Linux CC Args: x0, x1, x2, x3, x4, x5
// Syscall no: w8
InternalSyscallImpl:
    mov x8, x0
    mov x0, x1
    mov x1, x2
    mov x2, x3
    mov x3, x4
    mov x4, x5
    mov x5, x6
    svc #0
    ret
