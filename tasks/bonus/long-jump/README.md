# Long jump

В C/C++ есть необычный механизм передачи управления `setjmp`/`longjmp`,
который позволяет делать "нелокальный goto". `int setjmp(jmp_buf buf)` при вызове
записывает текущий контекст исполнения в аргумент `buf` и возвращает 0. Если далее
происходит вызов `longjmp(buf, value)`, управление передается в последний вызов
`setjmp`, который был вызван на данном `buf`, таким образом, чтобы он вернул
значение `value`. Например:

```cpp
jmp_buf buf;

if (setjmp(buf) == 123) {
    std::cout << "After jump" << std::endl;
    return;
}

std::cout << "Setup completed" << std::endl;

longjmp(buf, 123);

std::cout << "After jump" << std::endl;
```

Выведет на экран

```plain
Setup completed
After jump
```

Этот механизм можно использовать, как ограниченную, но почти zero-cost, замену исключениям:

```cpp
Result DoComputation(Args args, jmp_buf* err_handler) {
    // ...
    if (error) {
        longjmp(*err_handler, error.code());
    }
    // ...
}

jmp_buf err_handler;

switch (setjmp(err_handler)) {
    case 0:
    return DoComutation(..., &err_handler);
    case ErrorCode1:
    // Handle errors
    case ErrorCode2:
    // ...
}
```

Однако этот механизм имеет довольно много ограничений.
В частности, `setjmp` должен использоваться как отдельно стоящий вызов функции,
либо быть всей управляющей частью `if/switch/while/do-while/for`,
либо одним из операндов сравнения/проверки на равенство с целочисленной константой
так, чтобы результат был всей управляющей частью одного из перечисленных операторов.

`longjmp` не может "пропускать" нетривиальные деструкторы. Подробнее об ограничениях на
использование `setjmp`/`longjmp` вы можете почитать на `cppreference` и `manpages`:

- [`cppreference`](https://en.cppreference.com/w/cpp/utility/program/setjmp)
- [`manpages`](https://www.man7.org/linux/man-pages/man3/setjmp.3.html)


## Задача

Ваша задача – реализовать аналогичный механизм. Так как реализовать его на "чистом" C/C++ невозможно,
придется реализовывать его на ассемблере.

Реализуйте структуру `JumpBuf` в файле [`long-jump.hpp`](./src/long-jump.hpp) и функции
`SetJump`, `LongJump` в файле [`long-jump.S`](./src/long-jump.S). Вы также можете создавать новые
`.cpp`/`.hpp`/`.S` файлы, если посчитаете это нужным.

Гарантируется, что аргумент `value` у `LongJump` не равен 0.

## Playground

В файле [`playground/main.cpp`](./playground/main.cpp) вы можете поэкспериментировать с
результатом своей работы. Для его запуска используйте `cargo xtask play`.

## Возможная реализация

<details>
<summary>Спойлеры</summary>

Как должен работать `LongJump`? По всей видимости, ему нужно восстановить
`rsp` и `rip` на момент выполнения `SetJump`, это вернет исполнение в момент
вызова `SetJump`, но скорее всего вызовет проблемы с ожиданием компилятора на
состояние callee-saved регистров. Поэтому нужно также восстановить и их –
сохраните состояние всех нужных регистров в `JumpBuf` в `SetJump` и восстановите
их значения в `LongJump`. Для сохраниения/восстановления регистра `rip` удобно
использовать адрес возврата.
</details>
