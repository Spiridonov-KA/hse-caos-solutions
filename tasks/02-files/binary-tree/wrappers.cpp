#include <glitch.hpp>

#include <pcg-random.hpp>

#include <cstdint>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

struct ReadGlitches final : ReadGuard, PReadGuard, ReadChkGuard, PReadChkGuard {
    ReadGlitches() : rng_(424243) {
        static constexpr int64_t kNsInSec = 1'000'000'000;

        const char* delay_str = getenv("READ_DELAY_NS");
        long long total_ns = 0;
        if (delay_str) {
            total_ns = strtoll(delay_str, NULL, 10);
        }
        read_delay_ = {
            .tv_sec = total_ns / kNsInSec,
            .tv_nsec = total_ns % kNsInSec,
        };

        const char* req_str = getenv("MIN_READ_CALLS");
        min_read_calls_ = 1;
        if (req_str) {
            min_read_calls_ = strtoll(req_str, NULL, 10);
        }
    }

    ssize_t Read(int fd, void* buf, size_t count) override {
        OnRead(&count);
        return RealRead(fd, buf, count);
    }

    ssize_t PRead(int fd, void* buf, size_t count, off_t off) override {
        OnRead(&count);
        return RealPRead(fd, buf, count, off);
    }

    ssize_t ReadChk(int fd, void* buf, size_t count, size_t buflen) override {
        OnRead(&count);
        return RealReadChk(fd, buf, count, buflen);
    }

    ssize_t PReadChk(int fd, void* buf, size_t count, off_t off,
                     size_t buflen) override {
        OnRead(&count);
        return RealPReadChk(fd, buf, count, off, buflen);
    }

    ~ReadGlitches() {
        CheckReadCalls();
    }

  private:
    void MaybeSleep() const {
        if (read_delay_.tv_sec > 0 || read_delay_.tv_nsec > 0) {
            nanosleep(&read_delay_, NULL);
        }
    }

    void TweakReadCnt(size_t* count) {
        if (rng_() % 10 != 0 || *count == 0) {
            return;
        }
        (*count) = rng_() % (*count) + 1;
    }

    void OnRead(size_t* count) {
        MaybeSleep();
        TweakReadCnt(count);
        if (*count != 0) {
            ++read_calls_;
        }
    }

    void CheckReadCalls() {
        if (read_calls_ >= min_read_calls_) {
            return;
        }
        std::cerr << "Not enough (p)read calls: at least " << min_read_calls_
                  << " expected, actual is " << read_calls_ << std::endl;
        abort();
    }

    PCGRandom rng_;
    timespec read_delay_;

    uint64_t min_read_calls_;
    uint64_t read_calls_ = 0;
};

static ReadGlitches read_glitches;
