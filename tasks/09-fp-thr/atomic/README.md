# Atomic

Реализуйте атомарные операции
[`AtomicStore`](https://cppreference.com/w/cpp/atomic/atomic/store.html),
[`AtomicLoad`](https://cppreference.com/w/cpp/atomic/atomic/load.html),
[`AtomicFetchAdd`](https://cppreference.com/w/cpp/atomic/atomic/fetch_add.html) и
[`AtomicExchange`](https://cppreference.com/w/cpp/atomic/atomic/exchange.html)
в файле [`atomic.S`](./atomic.S).

Посмотрите, в какие инструкции компилируются соответствующие операции у `std::atomic`.
Попробуйте запустить тесты с наивной реализацией `AtomicStore` и `AtomicLoad`.
