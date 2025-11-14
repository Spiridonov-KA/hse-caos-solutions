#pragma once

#include <cstddef>
#include <utility>

class Stack {
  public:
    static Stack Allocate(size_t pages);
    ~Stack();

    Stack() = default;
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    Stack(Stack&& other) : Stack() {
        Swap(other);
    }

    Stack& operator=(Stack&& other) {
        Stack tmp{std::move(other)};
        Swap(tmp);
        return *this;
    }

    void Swap(Stack& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(len_, other.len_);
    }

    void* End() {
        return (char*)ptr_ + len_;
    }

    bool IsEmpty() const {
        return len_ == 0;
    }

  private:
    Stack(void* ptr, size_t len) : ptr_(ptr), len_(len) {
    }

    void* ptr_ = nullptr;
    size_t len_ = 0;
};
