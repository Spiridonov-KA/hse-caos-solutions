# Syscalls

В файле [`./syscalls.cpp`](./syscalls.cpp) реализуйте обертки для системных
вызовов с помощью
[GNU inline assembly](https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html).

Так как gnu inline assembly не поддерживает указание записи аргументов в регистры
`r8-r15`, вам, скорее всего, придется пойти по одному из двух путей:

1. Передать аргументы через произвольные регистры и переложить их в нужные регистры
в ассемблерной вставке. Вам может помочь следующая гарантия от компилятора:

    > The input operands are guaranteed not to use any of the clobbered registers,
    > and neither do the output operands' addresses.

    https://gcc.gnu.org/onlinedocs/gcc-4.8.2/gcc/Extended-Asm.html

1. Указать компилятору регистры, в которые нужно положить аргументы, используя
*explicit register variables*.
Этот способ менее портируемый, так как является gnu extension и не является частью стандарта.
Однако сейчас его использование для наших целей поддерживается
компилятором gcc/g++, но не факт, что поддерживается другими.

    https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html

Обратите внимание, что ваш код будет собираться с флагом `-masm=intel`, поэтому в ассемблерных
вставках стоит использовать intel-синтаксис в отличие от большинства документации по gnu inline
assembly, где используется AT&T синтаксис.

Также помните, что инструкция `syscall` [портит](../../../docs/asm-cheatsheet.md#linux-x86_64-syscall-calling-conventions) регистры `rcx`, `r11`.

## Полезные ссылки

- [man 2 syscall](https://www.man7.org/linux/man-pages/man2/syscall.2.html)
- [syscall instruction reference](https://www.felixcloutier.com/x86/syscall)
