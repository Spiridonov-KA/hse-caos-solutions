.intel_syntax noprefix
.section .note.GNU-stack

.text

.global Process
Process:
    push    rbp
    mov     rbp, rsp

    push    rbp
    mov     rbp, rsp

    mov     rax, [rbp + 16]
    mov     [rbp - 8], rdi
    mov     [rbp - 16], rsi
    mov     [rbp - 24], rdx
    mov     [rbp - 32], rcx
    mov     [rbp - 40], r8
    mov     [rbp - 48], r9

    mov     rax, [rbp - 8]
    add     rax, [rbp - 16]
    add     rax, [rbp - 24]
    add     rax, [rbp - 32]
    add     rax, [rbp - 40]
    add     rax, [rbp - 48]
    add     rax, [rbp + 16]
    pop     rbp
    ret
