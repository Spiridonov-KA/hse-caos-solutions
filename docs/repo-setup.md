# Настройка репозитория

## Пререквизиты

- [Рабочее окружение](./env-setup.md)

## Создание репозитория

Напишите телеграм боту [hse_caos_2025_bot](https://t.me/hse_caos_2025_bot) для получения логина/пароля от [gitlab.hse-caos.org](https://gitlab.hse-caos.org). В этом gitlab будет храниться ваш репозиторий с решениями, а так же будет происходить тестирование домашних заданий. Далее на странице [manytask](https://manytask.hse-caos.org) нажмите на `Login with` и зайдите в свой аккаунт на gitlab. После этого в manytask вбейте в поиск название курса: `hse-caos-2025`, или перейдите [по ссылке](https://manytask.hse-caos.org/hse-caos-2025/) и введите секрет из канала курса.

Основным публичным репозиторием курса будет [hse-caos-public](https://gitlab.com/oleg-shatov/hse-caos-public), все обновления и домашние задания будут публиковаться там.

### Настройка ssh-ключа

Для работы с gitlab вам понадобиться добавить туда свой публичный ssh ключ. Проверить его наличие можно посмотрев в директорию `~/.ssh` на предмет файлов с названием `id_*.pub`:

```plain
$ ls ~/.ssh
config  id_ecdsa  id_ecdsa.pub
                  ^^^^^^^^^^^^
```

Его содержимое нужно добавить в свой профиль в gitlab: `иконка пользователя` -> `preferences` -> `ssh keys` -> `add new key`.

Если публичного ключа у вас нет, его можно сгенерировать командой

```bash
ssh-keygen -t ed25519 -C "<email>"
```

### Клонирование

В ui gitlab скопируйте ссылку для клонирования репозитория **через ssh** (`code` -> `clone with ssh`). Для клонирования репозитория выполните

```bash
git clone ssh://git@gitlab.hse-caos.org:2025/hse-caos-fall/students-2025/<username>.git
```

На вопрос `Are you sure you want to continue connecting (yes/no/[fingerprint])?`, если он есть, ответьте yes.

Перейдите в папку склонированного репозитория:

```bash
cd <username>
```

Добавьте публичный репозиторий, как второй remote для доставки обновлений:

```bash
git remote add upstream https://gitlab.com/oleg-shatov/hse-caos-public.git
#              ^^^^^^^^ это название remote, которое вы должны будете использовать
#                       в git pull при обновлениях
```

### Обновление репозитория

В публичном репозитории будут регулярно появляться новые задачи, для обновления своего репозитория используйте

```bash
git pull upstream main --no-rebase
```

После настройки репозитория можете приступать к [сдаче задач](./submit.md).
