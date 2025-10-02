#include "capture.hpp"

#include <distributions.hpp>
#include <fd_guard.hpp>
#include <overload.hpp>
#include <rlim_guard.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <iostream>
#include <random>

#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>

void Flush() {
    std::cout.flush();
    std::clog.flush();
    std::fflush(stdout);
    std::fflush(stderr);
}

template <class Rng>
std::string GenerateStr(Rng& rng, size_t n) {
    UniformCharDistribution dist('a', 'z');
    std::string s(n, ' ');
    std::generate(s.begin(), s.end(), [&rng, &dist] { return dist(rng); });
    return s;
}

void ResetCinState() {
    std::cin.clear();
    std::clearerr(stdin);
}

static constexpr size_t kPipeSize = 4 << 10;

TEST_CASE("CheckPipeCapacity") {
    INFO("This is an internal assertion, please report if it fails");
    int fds[2];
    int r = pipe(fds);
    REQUIRE(r != -1);
    int cap = fcntl(fds[0], F_GETPIPE_SZ);
    {
        int err = errno;
        INFO("Errno is " << err);
        REQUIRE(cap != -1);
    }
    REQUIRE(static_cast<size_t>(cap) >= kPipeSize);
    close(fds[0]);
    close(fds[1]);
}

TEST_CASE("JustWorks") {
    FileDescriptorsGuard guard;

    SECTION("Simple") {
        int runs = 0;
        Flush();
        auto result = CaptureOutput([&runs] { ++runs; }, "");
        REQUIRE(result.index() == 0);
        auto [out, err] = std::get<0>(std::move(result));
        CHECK(out == "");
        CHECK(err == "");
        CHECK(runs == 1);
        CHECK(guard.TestDescriptorsState());
    }

    SECTION("Output") {
        Flush();
        auto result = CaptureOutput(
            [] {
                std::cout << "Aba";
                std::cout.flush();
                std::cerr << "Caba";
            },
            "");
        REQUIRE(result.index() == 0);
        auto [out, err] = std::get<0>(std::move(result));
        CHECK(out == "Aba");
        CHECK(err == "Caba");
        CHECK(guard.TestDescriptorsState());
    }

    SECTION("Input+Output") {
        Flush();
        auto result = CaptureOutput(
            [] {
                for (size_t i = 0; i < 3; ++i) {
                    int v;
                    std::cin >> v;
                    if (v & 1) {
                        std::cout << v << " ";
                    } else {
                        std::cerr << v << " ";
                    }
                }

                std::cout.flush();
            },
            "1 2 3 ");

        REQUIRE(result.index() == 0);
        auto [out, err] = std::get<0>(std::move(result));
        CHECK(out == "1 3 ");
        CHECK(err == "2 ");
        CHECK(guard.TestDescriptorsState());
    }
}

TEST_CASE("EOF") {
    FileDescriptorsGuard guard;

    Flush();
    auto result = CaptureOutput(
        [] {
            int v;
            while (std::cin >> v) {
                if (v & 1) {
                    std::cout << v;
                } else {
                    std::cerr << v;
                }
            }
            std::cout.flush();
        },
        "1 2  3 4 5  6 7 8");
    ResetCinState();

    REQUIRE(result.index() == 0);
    auto [out, err] = std::get<0>(std::move(result));
    CHECK(out == "1357");
    CHECK(err == "2468");
    CHECK(guard.TestDescriptorsState());
}

TEST_CASE("HugeIO") {
    FileDescriptorsGuard guard;

    static constexpr size_t kBufSize = kPipeSize;
    std::mt19937 rng(Catch::getSeed());

    auto inp = GenerateStr(rng, kBufSize);
    auto my_out = GenerateStr(rng, kBufSize);
    auto my_err = GenerateStr(rng, kBufSize);

    std::string actual_inp;
    Flush();
    auto result = CaptureOutput(
        [&] {
            std::string s;
            std::cin >> s;

            std::cout << my_out;
            std::cerr << my_err;

            actual_inp = std::move(s);
            std::cout.flush();
        },
        inp);
    ResetCinState();

    CHECK(actual_inp == inp);

    REQUIRE(result.index() == 0);
    auto [out, err] = std::get<0>(std::move(result));
    CHECK(out == my_out);
    CHECK(err == my_err);
    CHECK(guard.TestDescriptorsState());
}

TEST_CASE("ErrorRecovery") {
    std::mt19937 rng(Catch::getSeed());

    FileDescriptorsGuard guard;
    constexpr auto base_fd_count = 3;

    for (size_t i = 0; i <= 10; ++i) {
        INFO("i = " << i);

        auto out = GenerateStr(rng, 10);
        auto err = GenerateStr(rng, 10);
        auto inp = GenerateStr(rng, 10);

        Flush();
        auto result = [&] {
            RLimGuard files_guard(RLIMIT_NOFILE, base_fd_count + i);
            return CaptureOutput(
                [&] {
                    (std::cout << out).flush();
                    std::cerr << err;
                },
                inp);
        }();

        if (i < 2) {
            REQUIRE(result.index() == 1);
        }
        if (i > 9) {
            REQUIRE(result.index() == 0);
        }

        std::visit(Overload{
                       [&](std::pair<std::string, std::string> output) {
                           CHECK(output.first == out);
                           CHECK(output.second == err);
                       },
                       [](int err) {
                           INFO("Error code: " << err << " "
                                               << std::strerror(err));
                           REQUIRE((err == EBADF || err == EMFILE));
                       },
                   },
                   std::move(result));

        CHECK(guard.TestDescriptorsState());
    }
}

TEST_CASE("RawIO") {
    constexpr size_t kBufSize = 10;

    std::mt19937 rng(Catch::getSeed());
    auto input = GenerateStr(rng, kBufSize);
    auto output = GenerateStr(rng, kBufSize);
    auto error = GenerateStr(rng, kBufSize);

    auto write_all = [](int fd, std::string_view data) -> int {
        while (!data.empty()) {
            int w = write(fd, data.data(), data.size());
            if (w == -1) {
                return -errno;
            }
            data = data.substr(w);
        }
        return 0;
    };

    char real_input[kBufSize + 1];

    int out_err = 0;
    int err_err = 0;
    int in_err = 0;
    auto result = CaptureOutput(
        [&] {
            out_err = write_all(STDOUT_FILENO, output);
            err_err = write_all(STDERR_FILENO, error);

            char* in = real_input;
            char* real_in_end = real_input + kBufSize;
            while (in < real_in_end) {
                int r = read(0, in, real_in_end - in);
                if (r == -1) {
                    in_err = -errno;
                    break;
                }
                if (r == 0) {
                    in_err = 1023;
                    break;
                }
                in += r;
            }

            if (read(0, in, 1) != 0) {
                in_err = 1024;
            }
        },
        input);

    REQUIRE(out_err == 0);
    REQUIRE(err_err == 0);
    REQUIRE(in_err == 0);

    REQUIRE(result.index() == 0);
    auto [out, err] = std::get<0>(result);

    CHECK(out == output);
    CHECK(err == error);
    CHECK(std::string_view{real_input, kBufSize} == input);
}
