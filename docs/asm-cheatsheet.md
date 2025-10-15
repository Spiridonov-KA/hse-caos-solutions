# x86_64 assembly cheatsheet

## General purpose registers and their parts

| 64-bit | 32-bit | 16-bit | 8 high bits of lower 16 bits | 8-bit | Description                             |
| ------ | ------ | ------ | ---------------------------- | ----- | --------------------------------------- |
| RAX    | EAX    | AX     | AH                           | AL    | Accumulator                             |
| RBX    | EBX    | BX     | BH                           | BL    | Base                                    |
| RCX    | ECX    | CX     | CH                           | CL    | Counter                                 |
| RDX    | EDX    | DX     | DH                           | DL    | Data (commonly extends the A register)  |
| RSI    | ESI    | SI     | N/A                          | SIL   | Source index for string operations      |
| RDI    | EDI    | DI     | N/A                          | DIL   | Destination index for string operations |
| RSP    | ESP    | SP     | N/A                          | SPL   | Stack Pointer                           |
| RBP    | EBP    | BP     | N/A                          | BPL   | Base Pointer (meant for stack frames)   |
| R8     | R8D    | R8W    | N/A                          | R8B   | General purpose                         |
| R9     | R9D    | R9W    | N/A                          | R9B   | General purpose                         |
| R10    | R10D   | R10W   | N/A                          | R10B  | General purpose                         |
| R11    | R11D   | R11W   | N/A                          | R11B  | General purpose                         |
| R12    | R12D   | R12W   | N/A                          | R12B  | General purpose                         |
| R13    | R13D   | R13W   | N/A                          | R13B  | General purpose                         |
| R14    | R14D   | R14W   | N/A                          | R14B  | General purpose                         |
| R15    | R15D   | R15W   | N/A                          | R15B  | General purpose                         |

## Scale-Index-Base (SIB) addressing

`[rbase + rindex * scale + displacement]`

- `rbase`, `rindex` – registers
- `scale`, `displacement` – constants, `scale ∈ {1, 2, 4, 8}`

### Examples

```asm
mov [rdi + rdx * 8 + 0x10], rax  # *(uint64_t*)(rdi + rdx * 8 + 0x10) = rax
mov rax, [rdi - 0x60]            # rax = *(uint64_t*)(rdi - 0x60)
```

## System V AMD64 ABI (Linux calling conventions)

Arguments: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`

Return value: `rax` or `rdx:rax` or stack

Caller-saved registers: `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`, `r9`, `r10`, `r11`

Callee-saved registers: `rbx`, `rsp`, `rbp`, `r12`, `r13`, `r14`, `r15`

Before `call`: `rsp % 16 == 0`

## Size qualifiers

- `BYTE PTR` – 8-bit
- `WORD PTR` – 16-bit
- `DWORD PTR` – 32-bit
- `QWORD PTR` – 64-bit
- `XMMWORD PTR` – 128-bit
- `YMMWORD PTR` – 256-bit
- `ZMMWORD PTR` – 512-bit

### Examples

```asm
mov BYTE PTR [rdx], 10              # *(uint8_t)rdx = 10
mov DWORD PTR [rsi + rdx * 8], 123  # *(uint32_t*)(rsi + rdx * 8) = 123
```

## Links

- [CPU Registers x86_64](https://wiki.osdev.org/CPU_Registers_x86-64)
- [x86_64 instructions reference](https://www.felixcloutier.com/x86/)
- [Intel software developers manual](https://cdrdv2.intel.com/v1/dl/getContent/671200)
