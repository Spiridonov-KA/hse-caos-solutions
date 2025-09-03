# Установка Rust

## Prerequisites

- [Linux](./linux.md)

## Необходимые пакеты

Для прохождения курса вам потребуются следующие утилиты:

- `git`
- `g++` или `clang++`
- `cmake`
- `clang-format`
- `curl` (для скачивания установщика `rustup`)

Установить на ubuntu их можно следующим образом:

```bash
apt-get update
apt-get install -y git g++ cmake clang-format curl
```

## Cargo

Если rust у вас уже установлен, переходите к шагу [Настройка](#настройка)

Инструментарий языка rust используется в нашем курсе для автоматизации тестирования. Для его установки выполните в терминале команду с [официального сайта](https://rustup.rs/):

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

В процессе установки вам будет дан выбор:

```plain
1) Proceed with standard installation (default - just press enter)
2) Customize installation
3) Cancel installation
```

Нам потребуется версия nightly, поэтому выбираем `2`. Далее вам предложат поменять default host triple, default toolchain, profile и предложат поменять `PATH`. Везде нужно выбрать значения по-умолчанию кроме шага "default toolchain", в нем нужно ввести "nightly", далее нажимаем <kbd>Enter</kbd>, завершая установку. Чтобы проверить, что все установилось, выполните

```bash
# Эта команда запускает процесс bash заново, позволяя bash
# найти только что установленные программы. Вместо нее можете
# просто открыть новое окно терминала.
exec bash

cargo --help
```

Вы должны увидеть что-то наподобии

```plain
Rust's package manager

Usage: cargo [+toolchain] [OPTIONS] [COMMAND]
       cargo [+toolchain] [OPTIONS] -Zscript <MANIFEST_RS> [ARGS]...

...
```

## Настройка

Если у вас уже был установлен rust, или вы в процессе установки не выбрали nightly, это можно поменять с помощью команды

```bash
rustup default nightly
```
