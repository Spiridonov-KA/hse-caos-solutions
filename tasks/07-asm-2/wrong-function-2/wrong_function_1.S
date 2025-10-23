.intel_syntax noprefix
.section .note.GNU-stack

.text

.global Process
Process:
    push rbp
    mov rbp, rsp

    push rbp
    mov rbp, rsp

    xor rax, rax
    add rax, rdi
    add rax, rsi
    add rax, rdx

    leave
    ret
