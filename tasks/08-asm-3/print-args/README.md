# Print args

Реализуйте программу, которая выводит свои аргументы командной строки и переменные
окружения в таком же формате, в котором их выведет следующая программа:

```cpp
#include <iostream>

extern char** environ;

int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << std::endl;
    }
    std::cout << std::endl;
    for (auto env = environ; *env; ++env) {
        std::cout << *env << std::endl;
    }
}
```

Ваш код будет собираться без стандартной библиотеки (флаги `-ffreestanding` и `-nostdlib`).
Вы можете создавать новые `.cpp` и `.S` файлы в директории `src`.

## Полезные ссылки

- [SysV ABI documentation](https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf) – на стр. 29 описано состояние адресного пространства на момент старта программы, включая расположение всех необходимых вам величин.
