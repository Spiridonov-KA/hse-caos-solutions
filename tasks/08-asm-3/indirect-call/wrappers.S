.intel_syntax noprefix

.section .note.GNU-stack

.section .rodata, "a", @progbits

fmt:
    .asciz "Register %s was not preserved"

#define REG_STR(reg) \
s##reg:              \
    .asciz #reg

REG_STR(rax)
REG_STR(rbx)
REG_STR(rcx)
REG_STR(rdx)
REG_STR(rdi)
REG_STR(rsi)
REG_STR(rbp)
REG_STR(r8)
REG_STR(r9)
REG_STR(r10)
REG_STR(r11)
REG_STR(r12)
REG_STR(r13)
REG_STR(r14)
REG_STR(r15)

#undef REG_STR

.bss
.global realG
realG:
    .quad 0
.global realH
realH:
    .quad 0

.macro Clobber
    mov rax, 123
    mov rcx, 124
    mov rdx, 125
    mov rsi, 126
    mov rdi, 127
    mov r8, 128
    mov r9, 129
    mov r10, 130
    mov r11, 131
.endm

.text
.global GWrapper
GWrapper:
    Clobber
    jmp [rip + realG]

.global HWrapper
HWrapper:
    Clobber
    jmp [rip + realH]

.global IndirectCallWrapper
IndirectCallWrapper:
    push rbx

    mov rbx, rsi
    mov rcx, rdx
    mov rax, rdi

    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push rbp
    push r8
    push r9
    push r10
    push r12
    push r13
    push r14
    push r15
    push r11

    call Indirect.Call

    cmp r11, [rsp]
    jne .r11_mismatch
    add rsp, 8

.macro CHECK_REG reg
    cmp \reg, [rsp]
    lea r11, [rip + s\reg]
    jne .mismatch
    add rsp, 8
.endm

    CHECK_REG r15
    CHECK_REG r14
    CHECK_REG r13
    CHECK_REG r12
    CHECK_REG r10
    CHECK_REG r9
    CHECK_REG r8
    CHECK_REG rbp
    CHECK_REG rsi
    CHECK_REG rdi
    CHECK_REG rdx
    CHECK_REG rcx
    CHECK_REG rbx

    pop rbx
    ret

.r11_mismatch:
    lea rdi, [rip + fmt]
    lea rsi, [rip + sr11]
    xor eax, eax
    call printf
    call abort

.mismatch:
    lea rdi, [rip + fmt]
    mov rsi, r11
    and rsp, -16
    xor eax, eax
    call printf
    call abort
