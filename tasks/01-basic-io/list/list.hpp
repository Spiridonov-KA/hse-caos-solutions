#pragma once

#include <utility>

#include <unused.hpp>  // TODO: remove before flight.

class List {
  public:
    // Non-copyable
    List(const List&) = delete;
    List& operator=(const List&) = delete;

    List(List&& other) : List() {
        Swap(other);
    }

    List& operator=(List&& other) {
        List tmp{std::move(other)};
        Swap(tmp);
        return *this;
    }

    List() {
    }

    ~List() {
        Clear();
    }

    void Clear() {
        // TODO: your code here.
    }

    void PushBack(int value) {
        UNUSED(value);  // TODO: remove before flight.
    }

    void PushFront(int value) {
        UNUSED(value);  // TODO: remove before flight.
    }

    void PopBack() {
        // TODO: your code here.
    }

    void PopFront() {
        // TODO: your code here.
    }

    int& Back() {
        // TODO: your code here.
        throw "TODO";
    }

    int& Front() {
        // TODO: your code here.
        throw "TODO";
    }

    bool IsEmpty() const {
        // TODO: your code here.
        throw "TODO";
    }

    void Swap(List& other) {
        UNUSED(other);  // TODO: remove before flight.
    }

    // https://en.cppreference.com/w/cpp/container/list/splice
    // Expected behavior:
    // l1 = {1, 2, 3};
    // l1.Splice({4, 5, 6});
    // l1 == {1, 2, 3, 4, 5, 6};
    void Splice(List& other) {
        UNUSED(other);  // TODO: remove before flight.
    }

    template <class F>
    void ForEachElement(F&& f) const {
        UNUSED(f);  // TODO: remove before flight.
    }

  private:
    // TODO: your code here.
};
