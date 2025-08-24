#pragma once

namespace detail {

void DoNotOptimize(const void*);

}

void DoNotReorder();

template <class T>
void DoNotOptimize(const T& v) {
    detail::DoNotOptimize(&v);
}
