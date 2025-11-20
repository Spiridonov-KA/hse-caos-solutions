#pragma once

#include <mutex>
#include <vector>

class Table {
  public:
    Table(size_t philosophers) : forks_(philosophers) {
    }

    void StartEating(size_t i);
    void StopEating(size_t i);

  private:
    size_t RightFork(size_t i) const;

    size_t LeftFork(size_t i) const;

    std::vector<std::mutex> forks_;
};
