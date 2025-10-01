#pragma once

#include "allocator.hpp"

#include <assert.hpp>

#include <cstddef>
#include <new>  // IWYU pragma: keep
#include <utility>

namespace detail {

template <class T>
struct Memory {
    Memory() noexcept = default;

    Memory(size_t size) : size_(size) {
        mem_ = Allocate(size * sizeof(T));
        ASSERT(mem_ != nullptr, "Faield to allocate memory");
    }

    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    Memory(Memory&& other) noexcept : Memory() {
        Swap(other);
    }

    Memory& operator=(Memory&& other) noexcept {
        Swap(other);
    }

    template <class... TArgs>
    void Emplace(TArgs&&... args) {
        new (GetElement(idx_)) T(std::forward<TArgs>(args)...);
        ++idx_;
    }

    auto GetElement(size_t i) noexcept {
        return (T*)((char*)mem_ + sizeof(T) * i);
    }

    auto GetElement(size_t i) const noexcept {
        return (const T*)((char*)mem_ + sizeof(T) * i);
    }

    void Pop() noexcept {
        GetElement(--idx_)->~T();
    }

    void Swap(Memory& other) noexcept {
        std::swap(mem_, other.mem_);
        std::swap(idx_, other.idx_);
        std::swap(size_, other.size_);
    }

    void Clear() {
        while (Size() > 0) {
            Pop();
        }
    }

    size_t Size() const noexcept {
        return idx_;
    }

    size_t Capacity() const noexcept {
        return size_;
    }

    ~Memory() {
        if (mem_) {
            for (size_t i = 0; i < Size(); ++i) {
                GetElement(i)->~T();
            }
            Deallocate(mem_);
        }
    }

  private:
    void* mem_ = nullptr;
    size_t idx_ = 0;
    size_t size_ = 0;
};

}  // namespace detail

template <class T>
class Vector {
  public:
    Vector() = default;

    Vector(size_t n) : mem_(n) {
        for (size_t i = 0; i < n; ++i) {
            mem_.Emplace();
        }
    }

    Vector(size_t n, const T& value) : mem_(n) {
        for (size_t i = 0; i < n; ++i) {
            mem_.Emplace(value);
        }
    }

    Vector(const Vector& other) : Vector() {
        *this = other;
    }

    Vector(Vector&& other) : Vector() {
        Swap(other);
    }

    Vector& operator=(const Vector& other) {
        Reserve(other.Size());
        for (size_t i = 0; i < Size() && i < other.Size(); ++i) {
            operator[](i) = other[i];
        }
        if (other.Size() > Size()) {
            for (size_t i = Size(); i < other.Size(); ++i) {
                PushBack(other[i]);
            }
        } else {
            while (Size() > other.Size()) {
                PopBack();
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& other) {
        Swap(other);
        other.Clear();
        return *this;
    }

    void PushBack(const T& value) {
        Reserve(Size() + 1);
        mem_.Emplace(value);
    }

    void PushBack(T&& value) {
        Reserve(Size() + 1);
        mem_.Emplace(std::move(value));
    }

    void PopBack() {
        mem_.Pop();
    }

    T& Front() {
        return operator[](0);
    }

    const T& Front() const {
        return operator[](0);
    }

    T& Back() {
        return operator[](Size() - 1);
    }

    const T& Back() const {
        return operator[](Size() - 1);
    }

    T& operator[](size_t idx) {
        return *mem_.GetElement(idx);
    }

    const T& operator[](size_t idx) const {
        return *mem_.GetElement(idx);
    }

    void Clear() {
        mem_.Clear();
    }

    void Swap(Vector& other) {
        mem_.Swap(other.mem_);
    }

    void Reserve(size_t n) {
        if (n <= Capacity()) {
            return;
        }
        if (n < 2 * Capacity()) {
            n = 2 * Capacity();
        }
        ReserveImpl(n);
    }

    void Resize(size_t n) {
        if (Size() < n) {
            Reserve(n);
            while (Size() < n) {
                mem_.Emplace();
            }
        } else {
            while (Size() > n) {
                mem_.Pop();
            }
        }
    }

    void Resize(size_t n, const T& value) {
        if (Size() < n) {
            Reserve(n);
            while (Size() < n) {
                mem_.Emplace(value);
            }
        } else {
            while (Size() > n) {
                mem_.Pop();
            }
        }
    }

    size_t Capacity() const noexcept {
        return mem_.Capacity();
    }

    size_t Size() const noexcept {
        return mem_.Size();
    }

    // https://devblogs.microsoft.com/cppblog/cpp23-deducing-this/
    T* begin() {
        return mem_.GetElement(0);
    }

    T* end() {
        return mem_.GetElement(mem_.Size());
    }

    const T* begin() const {
        return mem_.GetElement(0);
    }

    const T* end() const {
        return mem_.GetElement(mem_.Size());
    }

  private:
    void ReserveImpl(size_t n) {
        detail::Memory<T> mem_tmp(n);

        for (size_t i = 0; i < Size(); ++i) {
            mem_tmp.Emplace(std::move(*mem_.GetElement(i)));
        }

        mem_.Swap(mem_tmp);
    }

    detail::Memory<T> mem_;
};
