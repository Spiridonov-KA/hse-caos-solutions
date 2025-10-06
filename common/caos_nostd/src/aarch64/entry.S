.section .note.GNU-stack, "", @progbits

.section .text
.global _start
.align 16
_start:
    mov x29, #0  // Empty frame
    mov x30, #0

    mov x0, sp
    // NB: Entry has to be noreturn
    b Entry

.global Unreachable
.align 16  // Likely unnecessary
Unreachable:
    brk #0
