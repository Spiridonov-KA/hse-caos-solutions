.intel_syntax noprefix
.section .note.GNU-stack

.section .text

.global Process
Process:
    lea rdi, [rip + scans]
    lea rsi, [rip + tmp_arg]
    call scanf
    mov rax, [rip + tmp_arg]
    test rax, rax
    jz null
    jl negative
    shl rax, 1
    jmp done

negative:
    add rax, 7
    jmp done

null:
    mov rax, 42

done:
    mov rdx, 2
    mov rcx, 3
    leave
    ret

scans:
    .asciz "%ld"

.section .data
.global process_arg
process_arg:
    .int 0
tmp_arg:
    .int 0
