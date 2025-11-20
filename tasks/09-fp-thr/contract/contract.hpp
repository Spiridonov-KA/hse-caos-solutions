#pragma once

#include <utility>

#include <unused.hpp>  // TODO: remove before flight.

template <class T>
struct Future;

template <class T>
struct Promise {
    template <class U>
    friend std::pair<Future<U>, Promise<U>> CreateContract();

    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    Promise(Promise&&) = default;
    Promise& operator=(Promise&&) = default;

    void Set(T value) && {
        UNUSED(value);  // TODO: remove before flight.
    }

  private:
};

template <class T>
struct Future {
    template <class U>
    friend std::pair<Future<U>, Promise<U>> CreateContract();

    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    Future(Future&&) = default;
    Future& operator=(Future&&) = default;

    T Get() && {
        // TODO: your code here.
        throw "TODO";
    }

  private:
    // TODO: your code here.
};

template <class T>
std::pair<Future<T>, Promise<T>> CreateContract() {
    // TODO: your code here.
    throw "TODO";
}
