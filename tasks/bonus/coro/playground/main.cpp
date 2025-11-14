#include <coroutine.hpp>
#include <iostream>

template <class T>
class Generator {
  public:
    static std::unique_ptr<Generator<T>>
    Create(std::function<void(Generator<T>*)> body) {
        return std::make_unique<Generator<T>>(std::move(body));
    }

    void Yield(T value) {
        value_.emplace(std::move(value));
        coro_->Suspend();
    }

    std::optional<T> Next() {
        coro_->Resume();
        std::optional<T> value = std::move(value_);
        value_.reset();
        return value;
    }

    Generator(std::function<void(Generator<T>*)> body)
        : coro_(Coroutine::Create(
              [this, body = std::move(body)](Coroutine*) { body(this); })) {
    }

  private:
    std::unique_ptr<Coroutine> coro_;
    std::optional<T> value_;
};

int main() {
    auto g = Generator<int>::Create([](auto self) {
        for (int i = 0; i < 10; ++i) {
            self->Yield(i);
        }
    });

    while (auto v = g->Next()) {
        std::cout << *v << std::endl;
    }
}
