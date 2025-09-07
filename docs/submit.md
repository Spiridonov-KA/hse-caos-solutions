# Сдача задач

Для тестирования решений используется `cargo xtask`. Реализуйте решение задачи [A + B](https://gitlab.com/oleg-shatov/hse-caos-public/-/tree/main/tasks/01-basic-io/a-plus-b) в файле [a-plus-b.cpp](https://gitlab.com/oleg-shatov/hse-caos-public/-/blob/main/tasks/01-basic-io/a-plus-b/a-plus-b.cpp).

Когда посчитаете свое решение корректным, запустите из корня репозитория

```bash
(cd tasks/01-basic-io/a-plus-b; cargo xtask test)
```

Либо из директории с задачей

```bash
cargo xtask test
```

После того, как все тесты пройдут, отправьте решение на тестирование

```bash
git add tasks/01-basic-io/a-plus-b/a-plus-b.cpp
git commit -m "Up a-plus-b"
git push
```

За ходом тестирования можете следить по ссылке `My submits` из [manytask](https://manytask.hse-caos.org/hse-caos-2025/).
