#pragma once

#include <utility>

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
        (void)value; // TODO: remove before flight.
        // TODO: your code here.
    }

    void PushFront(int value) {
        (void)value; // TODO: remove before flight.
        // TODO: your code here.
    }

    void PopBack() {
        // TODO: your code here.
    }

    void PopFront() {
        // TODO: your code here.
    }

    int& Back() {
        // TODO: your code here.
        throw "TODO";\n    }

    int& Front() {
        // TODO: your code here.
        throw "TODO";\n    }

    bool IsEmpty() const {
        // TODO: your code here.
        throw "TODO";\n    }

    void Swap(List& other) {
        (void)other; // TODO: remove before flight.
        // TODO: your code here.
    }

    // https://en.cppreference.com/w/cpp/container/list/splice
    // Expected behavior:
    // l1 = {1, 2, 3};
    // l1.Splice({4, 5, 6});
    // l1 == {1, 2, 3, 4, 5, 6};
    void Splice(List& other) {
        (void)other; // TODO: remove before flight.
        // TODO: your code here.
    }

    template <class F>
    void ForEachElement(F&& f) const {
        (void)f; // TODO: remove before flight.
        // TODO: your code here.
    }

  private:
    // TODO: your code here.
};
