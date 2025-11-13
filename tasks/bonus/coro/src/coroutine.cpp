#include <coroutine.hpp>

#include <cstdlib>

#include <unused.hpp>  // TODO: remove before flight.

void Coroutine::Run() {
    std::abort();  // TODO: remove before flight.
}

Coroutine::Coroutine(std::function<void(Coroutine*)> body, Stack stack)
{
    UNUSED(body, stack);  // TODO: remove before flight.
}

std::unique_ptr<Coroutine>
Coroutine::Create(std::function<void(Coroutine*)> body, size_t stack_pages) {
    UNUSED(body, stack_pages);  // TODO: remove before flight.
    return nullptr;  // TODO: remove before flight.
}

void Coroutine::Suspend() {
    // TODO: your code here.
}

void Coroutine::Resume() {
    // TODO: your code here.
}

bool Coroutine::IsFinished() const {
    // TODO: your code here.
    return true;  // TODO: remove before flight.
}
