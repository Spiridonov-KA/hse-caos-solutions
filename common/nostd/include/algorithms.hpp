#pragma once

#include <utility>

template <class It, class Rng>
void Shuffle(It begin, It end, Rng& rng) {
    using DifferenceT = decltype(end - begin);

    if (begin == end) {
        return;
    }

    DifferenceT elements = 1;
    for (auto it = begin + 1; it != end; ++it) {
        ++elements;
        DifferenceT idx = static_cast<DifferenceT>(rng()) % elements;
        std::swap(*(begin + idx), *it);
    }
}
