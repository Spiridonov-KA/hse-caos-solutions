.intel_syntax noprefix
.section .note.GNU-stack

.section .text

.global Call
Call:
    // Preserved registers (https://wiki.osdev.org/Calling_Conventions)
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rbx, 1
    mov r12, 12
    mov r13, 13
    mov r14, 14
    mov r15, 15

    call my.args.forward

    cmp rbx, 1
    jne wrong
    cmp r12, 12
    jne wrong
    cmp r13, 13
    jne wrong
    cmp r14, 14
    jne wrong
    cmp r15, 15

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    neg rax

    leave
    ret

wrong:
    call abort
