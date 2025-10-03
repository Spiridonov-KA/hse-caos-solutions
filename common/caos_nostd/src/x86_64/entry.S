.intel_syntax noprefix
.section .note.GNU-stack, "", @progbits

.section .text
.global _start
.align 16
_start:
    and rsp, -0x10
    // NB: Main should be noreturn
    call Main
    ud2
