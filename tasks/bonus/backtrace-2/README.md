# Backtrace возвращается

Программы, в которых происходит некорректный доступ к памяти, часто завершаются
по сигналу `SIGSEGV` (Segmentation fault). Часто при завершении программы по
`SIGSEGV` довольно тяжело понять, где именно произошла ошибка. Напишите обработчик
сигнала `SIGSEGV`, который:

- Выводит stacktrace до места, в котором произошел segmentation fault
- Возобновляет исполнение программы с инструкции следующей за той, на которой
    произошел segmentation fault

Реализуйте метод

```cpp
void SetupHandler(std::span<const Function> functions);
```

Принимающий список известных функций, для которых нужно выводить stacktrace.
При возникновении segmentation fault должна выводиться диагностика вида

```plain
SIGSEGV on address 0x660
f1
f2
f3
```

Где `f1`, `f2`, `f3` – названия функций в стеке вызовов в порядке от последнего
(в хронологическом порядке) вызова к первому.

Разрешено делать signal-unsafe вызовы в обработчике сигнала. Так же можете полагаться
на то, что инструкция, спровоцировавшая segmentation fault является одной из

```asm
mov [reg], reg
mov reg, [reg]
```

Весь код в задачи скомпилирован с `-fno-omit-frame-pointer`.

## Граница размотки стека

При определении границы размотки стека у вас может возникнуть желание воспользоваться
[знанием](https://codebrowser.dev/glibc/glibc/sysdeps/x86_64/start.S.html) об устройстве
точки входа в программу. Вероятно, это не сработает, т.к. часть кода в стеке вызовов
может быть скомпилирована без `-fno-omit-frame-pointer`, скорее всего придется использовать
другие эвристики.

## Возобновление исполнения после обработки сигнала

Для возобновления исполнения после получения сигнала, контекст исполнения
на момент получения сигнала (значения всех регистров и т.д.) сохраняются
перед вызовом обработчика сигнала. Обработчик сигнала, установленный через
`sigaction` может получить доступ до сохраненного контекста:

> ucontext
>
> This is a pointer to a ucontext_t structure, cast to
> void *.  The structure pointed to by this field contains
> signal context information that was saved on the user-space
> stack by the kernel; for details, see sigreturn(2).
> Further information about the ucontext_t structure can be
> found in getcontext(3) and signal(7).  Commonly, the
> handler function doesn't make any use of the third
> argument.

https://www.man7.org/linux/man-pages/man2/sigaction.2.html

## Полезные ссылки

- [ucontext_t implementation](https://elixir.bootlin.com/glibc/glibc-2.42.9000/source/sysdeps/x86_64/sys/ucontext.h#L149)
- [backward-cpp](https://github.com/bombela/backward-cpp)
