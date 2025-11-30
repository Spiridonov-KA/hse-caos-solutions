# Thread sanitizer

В некоторых задачах ваш код будет компилироваться и запускаться с
[thread sanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html).
На свежих версиях ядра программы скомпилированные с thread sanitizer'ом
могут падать с сообщением вида

```plain
FATAL: ThreadSanitizer: unexpected memory mapping 0x604056080000-0x604056095000
```

Скорее всего, причина не в вашем решении, а в измененных настройках
[ASLR](https://en.wikipedia.org/wiki/Address_space_layout_randomization)
на новых версиях ядра. Чтобы исправить ошибку в thread sanitizer, уменьшите
рандомизацию ASLR, выполнив

```bash
sudo sysctl vm.mmap_rnd_bits=28
echo vm.mmap_rnd_bits=28 | sudo tee /etc/sysctl.d/99-tsan-fix.conf
```
