.intel_syntax noprefix
.section .note.GNU-stack, "", @progbits

.section .text
.global _start
.align 16
_start:
    xor rbp, rbp  // Empty frame

    mov rdi, rsp
    push 0  // Return address
    // NB: Entry has to be noreturn
    jmp Entry

.global Unreachable
.align 16
Unreachable:
    ud2
