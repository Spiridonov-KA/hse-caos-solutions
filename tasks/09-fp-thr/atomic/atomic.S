.intel_syntax noprefix

.section .note.GNU-stack

.text

.global AtomicStore
AtomicStore:
    mov [rdi], rsi  // TODO: remove before flight.
    ret

.global AtomicLoad
AtomicLoad:
    // TODO: your code here.
    ret

.global AtomicFetchAdd
AtomicFetchAdd:
    // TODO: your code here.
    ret

.global AtomicExchange
AtomicExchange:
    mov rax, [rdi]  // TODO: remove before flight.
    mov [rdi], rsi  // TODO: remove before flight.
    ret
