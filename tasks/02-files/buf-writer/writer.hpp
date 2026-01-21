#pragma once

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string_view>
#include <unistd.h>

#include <unused.hpp>  // TODO: remove before flight.

struct BufWriter {
    BufWriter(int fd, size_t buf_capacity)
        : fd_{fd}, size_{0}, capacity_{buf_capacity} {
        buffer_ = new char[capacity_];
    }

    // Non-copyable
    BufWriter(const BufWriter&) = delete;
    BufWriter& operator=(const BufWriter&) = delete;

    // Non-movable
    BufWriter(BufWriter&&) = delete;
    BufWriter& operator=(BufWriter&&) = delete;

    int Write(std::string_view data) {
        if (size_ + data.size() <= capacity_) {
            std::memcpy(buffer_ + size_, data.data(), data.size());
            size_ += data.size();
            return 0;
        }
        if (int res = Flush(); res != 0) {
            return res;
        }

        if (data.size() > capacity_) {
            size_t tail_size = data.size() % capacity_;
            size_t big_chunk_size = data.size() - tail_size;

            if (int res = FlushArray(data.data(), big_chunk_size); res != 0) {
                return res;
            }

            std::memcpy(buffer_, data.data() + big_chunk_size, tail_size);
            size_ = tail_size;
        } else {
            std::memcpy(buffer_, data.data(), data.size());
            size_ = data.size();
        }

        return 0;
    }

    int Flush() {
        int res = FlushArray(buffer_, size_);
        size_ = 0;
        return res;
    }

    ~BufWriter() {
        Flush();
        delete[] buffer_;
    }

  private:
    int FlushArray(const char* buf, size_t count) {
        size_t written = 0;
        while (written < count) {
            ssize_t res = ::write(fd_, buf + written, count - written);
            if (res == -1) {
                return -errno;
            }
            written += static_cast<size_t>(res);
        }
        return 0;
    }

    int fd_;
    char* buffer_;
    size_t size_;
    size_t capacity_;
};