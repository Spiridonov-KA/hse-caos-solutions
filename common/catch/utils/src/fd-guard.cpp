#include <fd-guard.hpp>

#include <internal-assert.hpp>

#include <algorithm>
#include <filesystem>
#include <unistd.h>

#include <fcntl.h>
#include <linux/kcmp.h>
#include <sys/resource.h>
#include <sys/syscall.h>

namespace fs = std::filesystem;

static std::vector<int> GetOpenFileDescriptors() {
    std::vector<int> fds_candidates;
    for (auto dent : fs::directory_iterator{"/proc/self/fd"}) {
        fds_candidates.push_back(std::stoi(dent.path().filename()));
    }
    std::vector<int> fds;
    for (auto fd : fds_candidates) {
        int ret = fcntl(fd, F_GETFD);
        if (ret == -1) {
            continue;
        }
        fds.push_back(fd);
    }
    std::sort(fds.begin(), fds.end());

    return fds;
}

static size_t MaxFdNum() {
    struct rlimit limits{};

    int r = ::getrlimit(RLIMIT_NOFILE, &limits);
    INTERNAL_ASSERT(r != -1);
    // Probably should check `/proc/sys/fs/nr_open` instead
    return std::min(static_cast<size_t>(limits.rlim_cur), size_t{2048});
}

FileDescriptorsGuard::FileDescriptorsGuard() {
    auto fds = GetOpenFileDescriptors();
    if (fds.empty()) {
        return;
    }

    auto max_fd_num = MaxFdNum();
    INTERNAL_ASSERT(fds.back() + fds.size() < max_fd_num);

    auto cur_fd_num = static_cast<int>(max_fd_num - 1);
    for (auto fd : fds) {
        int ret = dup2(fd, cur_fd_num);
        INTERNAL_ASSERT(ret != -1);
        copies.emplace_back(fd, cur_fd_num);
        --cur_fd_num;
    }
}

bool FileDescriptorsGuard::TestDescriptorsState() const {
    for (auto [from, to] : copies) {
        if (!TestSameFd(from, to)) {
            WARN("Descriptors " << from << " and " << to
                                << " don't refer to the same file");
            return false;
        }
    }

    auto fds = GetOpenFileDescriptors();
    auto expected = OpenFdsCount();
    if (fds.size() != expected) {
        WARN("Descriptors count mismatch. Expected "
             << expected << ", actually " << fds.size());
        return false;
    }
    return true;
}

size_t FileDescriptorsGuard::OpenFdsCount() const {
    return copies.size() * 2;
}

bool FileDescriptorsGuard::TestSameFd(int fd1, int fd2) const {
    int pid = getpid();
    int ret = syscall(SYS_kcmp, pid, pid, KCMP_FILE, fd1, fd2);
    if (ret == -1) {
        int err = errno;
        if (err == ENOSYS) {
            if (ShouldWarnKCmp()) {
                WARN("KCmp is not available on your system, some checks "
                     "wouldn't be performed");
            }
            return true;
        }
        INTERNAL_ASSERT(err == EBADF);
        return false;
    }
    return ret == 0;
}

bool FileDescriptorsGuard::ShouldWarnKCmp() const {
    return !std::exchange(warned_kcmp_, true);
}

FileDescriptorsGuard::~FileDescriptorsGuard() {
    for (auto [_, fd] : copies) {
        close(fd);
    }
}
