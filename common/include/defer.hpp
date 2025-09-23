#pragma once

#include <utility>

template <class F>
struct Defer {
    Defer(F&& f) : f_(std::forward<F>(f)) {
    }

    ~Defer() {
        f_();
    }

  private:
    F f_;
};
