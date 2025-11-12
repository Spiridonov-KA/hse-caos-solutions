.intel_syntax noprefix
.section .note.GNU-stack

.text
# uint64_t Indirect.Call(
#     uint64_t (*rax)(uint64_t, uint64_t),
#     uint64_t (*rbx)(),
#     uint64_t (*rcx)()
# );
.global Indirect.Call
Indirect.Call:
    // TODO: your code here.
    ret
