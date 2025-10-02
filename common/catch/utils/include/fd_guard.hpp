#pragma once

#ifndef __linux__
#error "Only linux is supported for this problem"
#endif

#include <cstddef>
#include <utility>
#include <vector>

struct FileDescriptorsGuard {
    FileDescriptorsGuard();

    [[nodiscard]] bool TestDescriptorsState() const;

    size_t OpenFdsCount() const;

    ~FileDescriptorsGuard();

  private:
    bool ShouldWarnKCmp() const;
    bool TestSameFd(int fd1, int fd2) const;

    std::vector<std::pair<int, int>> copies;
    mutable bool warned_kcmp_{false};
};
