#pragma once

#ifndef __linux__
#error "Only linux is supported for this problem"
#endif

#include <utility>
#include <vector>

struct FileDescriptorsGuard {
    FileDescriptorsGuard();

    [[nodiscard]] bool TestDescriptorsState() const;

    ~FileDescriptorsGuard();

  private:
    std::vector<std::pair<int, int>> copies;
};
