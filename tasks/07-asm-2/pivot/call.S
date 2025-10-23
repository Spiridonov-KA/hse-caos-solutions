.intel_syntax noprefix
.section .note.GNU-stack

.text
.align 16
.global Call

// rdi - rbx - arr
// rsi - esi - size
// rdx - edi - pivot
Call:
        push    rbp
        mov     rbp, rsp
        push    rbx
        push    rdi

        mov     rbx, rdi
        mov     rdi, rdx
        call    Partition
        mov     eax, edx

        pop     rcx
        cmp     rbx, rcx
        jnz     fail

        pop     rbx
        pop     rbp
        ret
fail:
        sub     rsp, 8
        lea     rdi, [rip + msg]
        mov     rsi, [rip + stderr]
        call    fputs
        call    abort

.section .rodata, "a", @progbits
msg:
        .asciz  "Subroutine messed up rbx\n"
