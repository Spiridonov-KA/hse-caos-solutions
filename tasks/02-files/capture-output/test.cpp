#include "capture.hpp"

#include <fd_guard.hpp>
#include <overload.hpp>
#include <rlim_guard.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <random>

#define CHECK_GUARD(guard)                                                     \
    do {                                                                       \
        auto check = guard.TestDescriptorsState();                             \
        CHECK(check);                                                          \
    } while (false)

void Flush() {
    std::cout.flush();
    std::clog.flush();
    std::fflush(stdout);
    std::fflush(stderr);
}

void ResetCinState() {
    std::cin.clear();
    std::clearerr(stdin);
}

TEST_CASE("CheckPipeCapacity") {
    INFO("This is an internal assertion, please report if it fails");
    int fds[2];
    int r = pipe(fds);
    REQUIRE(r != -1);
    int cap = fcntl(fds[0], F_GETPIPE_SZ);
    REQUIRE(cap >= (64 << 10));
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
        CHECK_GUARD(guard);
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
        CHECK_GUARD(guard);
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
        CHECK_GUARD(guard);
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
    CHECK_GUARD(guard);
}

TEST_CASE("HugeIO") {
    FileDescriptorsGuard guard;

    static constexpr size_t kBufSize = 64 << 10;
    std::mt19937 rng(Catch::getSeed());
    std::uniform_int_distribution<char> distr('a', 'z');

    auto create_string = [&] {
        std::string s(kBufSize, ' ');
        std::generate(s.begin(), s.end(), [&] { return distr(rng); });
        return s;
    };

    auto inp = create_string();
    auto my_out = create_string();
    auto my_err = create_string();

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
    CHECK_GUARD(guard);
}

TEST_CASE("ErrorRecovery") {
    static constexpr int kBaseFdCount = 6;  // 3 from the guard

    std::mt19937 rng(Catch::getSeed());
    std::uniform_int_distribution<char> distr('a', 'z');

    auto create_string = [&](size_t len) {
        std::string s(len, ' ');
        std::generate(s.begin(), s.end(), [&] { return distr(rng); });
        return s;
    };

    FileDescriptorsGuard guard;

    for (int i = 0; i <= 10; ++i) {
        RLimGuard files_guard(RLIMIT_NOFILE, kBaseFdCount + i);
        auto out = create_string(10);
        auto err = create_string(10);
        auto inp = create_string(10);

        Flush();
        auto result = CaptureOutput(
            [&] {
                (std::cout << out).flush();
                std::cerr << err;
            },
            inp);

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

        CHECK_GUARD(guard);
    }
}
