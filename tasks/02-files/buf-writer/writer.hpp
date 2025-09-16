#pragma once

#include <string_view>
#include <unistd.h>

#include <unused.hpp>  // TODO: remove before flight.

struct BufWriter {
    BufWriter(int fd, size_t buf_capacity) : fd_(fd) {
        UNUSED(buf_capacity);  // TODO: remove before flight.
    }

    // Non-copyable
    BufWriter(const BufWriter&) = delete;
    BufWriter& operator=(const BufWriter&) = delete;

    // Non-movable
    BufWriter(BufWriter&&) = delete;
    BufWriter& operator=(BufWriter&&) = delete;

    int Write(std::string_view data) {
        UNUSED(data, fd_);  // TODO: remove before flight.
        // TODO: your code here.
        return 0;
    }

    int Flush() {
        // TODO: your code here.
        return 0;
    }

    ~BufWriter() {
        // Best effort
        Flush();
    }

  private:

    int fd_;
    // TODO: your code here.
};
