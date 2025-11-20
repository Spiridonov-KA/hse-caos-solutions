#include "table.hpp"

void Table::StartEating(size_t i) {
    forks_[LeftFork(i)].lock();  // TODO: remove before flight.
    forks_[RightFork(i)].lock();  // TODO: remove before flight.
}

void Table::StopEating(size_t i) {
    forks_[LeftFork(i)].unlock();  // TODO: remove before flight.
    forks_[RightFork(i)].unlock();  // TODO: remove before flight.
}

size_t Table::RightFork(size_t i) const {
    ++i;
    if (i == forks_.size()) {
        i = 0;
    }
    return i;
}

size_t Table::LeftFork(size_t i) const {
    return i;
}
