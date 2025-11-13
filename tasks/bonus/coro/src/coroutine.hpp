#pragma once

#include <machine-context.hpp>
#include <stack.hpp>

#include <functional>
#include <memory>

class Coroutine final : IRunnable {
  public:
    // Non-copyable
    Coroutine(const Coroutine&) = delete;
    Coroutine& operator=(const Coroutine&) = delete;

    // Non-movable
    Coroutine(Coroutine&&) = delete;
    Coroutine& operator=(Coroutine&&) = delete;

    static std::unique_ptr<Coroutine>
    Create(std::function<void(Coroutine*)> body, size_t stack_pages = 16);

    void Resume();
    void Suspend();
    bool IsFinished() const;

    Coroutine(std::function<void(Coroutine*)> body, Stack stack);

  private:
    [[noreturn]] void Run() final;

    // TODO: your code here.
};
