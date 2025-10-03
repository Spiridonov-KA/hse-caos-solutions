.section .note.GNU-stack, "", @progbits

.section .text
.global _start
.align 16
_start:
    mov x0, sp
    bic x0, x0, #0xF
    mov sp, x0

    // NB: Main should be noreturn
    bl Main
    brk #0
