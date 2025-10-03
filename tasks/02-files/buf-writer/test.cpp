#include "writer.hpp"

#include <glitch.hpp>

#include <distributions.hpp>
#include <internal-assert.hpp>
#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <utility>

struct WriteGlitch final : WriteGuard {
    WriteGlitch(int fd, bool trim = false) : target_fd(fd), enable_trim(trim) {
    }

    ssize_t Write(int fd, const void* buf, size_t count) override {
        if (fd == target_fd) {
            ++writes_count;
            if (auto next = std::exchange(next_error, 0)) {
                errno = next;
                return -1;
            }

            if (enable_trim && count > 0) {
                count = rng() % count + 1;
            }
        }

        return RealWrite(fd, buf, count);
    }

    int target_fd;
    size_t writes_count = 0;
    int next_error = 0;
    bool enable_trim = false;
    PCGRandom rng{424243};
};

TEST_CASE("JustWorks") {
    int fds[2];
    int ret = pipe(fds);
    INTERNAL_ASSERT(ret != -1);

    {
        BufWriter w(fds[1], 100);
        w.Write("aba");
        w.Write("caba");
        w.Write("daba");
    }

    close(fds[1]);

    char data[11];
    int r = read(fds[0], data, sizeof(data));
    INTERNAL_ASSERT(r == sizeof(data));
    REQUIRE(std::string_view{data, sizeof(data)} == "abacabadaba");
    close(fds[0]);
}

TEST_CASE("CorrectNumberOfWrites") {
    int fds[2];
    int ret = pipe(fds);
    INTERNAL_ASSERT(ret != -1);
    {
        WriteGlitch g{fds[1]};
        BufWriter w(fds[1], 3);

        w.Write("ab");
        w.Write("abc");  // +1
        w.Write("abc");  // +1
        w.Write("abc");  // +1
        w.Flush();       // +1
        REQUIRE(g.writes_count == 4);
        w.Flush();
        REQUIRE(g.writes_count == 4);
    }

    {
        WriteGlitch g{fds[1]};
        BufWriter w{fds[1], 1000};

        w.Write("aba");
        w.Write("naba");
        w.Write("123");
        REQUIRE(g.writes_count == 0);
        w.Flush();
        REQUIRE(g.writes_count == 1);
    }

    close(fds[1]);

    char buf[21];
    int r = read(fds[0], buf, sizeof(buf));
    REQUIRE(r == sizeof(buf));
    REQUIRE(std::string_view{buf, sizeof(buf)} == "ababcabcabcabanaba123");

    close(fds[0]);
}

TEST_CASE("ErrorPropagation") {
    int fds[2];
    int ret = pipe(fds);
    REQUIRE(ret != -1);

    SECTION("Write") {
        WriteGlitch g{fds[1]};
        g.next_error = EIO;

        BufWriter w(fds[1], 3);

        int wr = w.Write("");
        REQUIRE(wr >= 0);
        wr = w.Write("a");
        REQUIRE(wr >= 0);
        CHECK(g.writes_count == 0);

        wr = w.Write("cab");
        CHECK(wr == -EIO);
    }

    SECTION("Flush") {
        WriteGlitch g{fds[1]};
        g.next_error = ENOSPC;
        BufWriter w(fds[1], 3);

        int wr = w.Write("a");
        REQUIRE(wr >= 0);
        wr = w.Flush();
        REQUIRE(wr == -ENOSPC);
    }

    close(fds[0]);
    close(fds[1]);
}

TEST_CASE("IncompleteWrites") {
    static constexpr size_t kBufCapacity = 5;
    PCGRandom rng{42424243};

    int fds[2];
    int r = pipe(fds);
    REQUIRE(r != -1);
    WriteGlitch g{fds[1], true};

    std::string data;
    {
        BufWriter w(fds[1], kBufCapacity);

        UniformCharDistribution distr('a', 'z');
        for (size_t i = 0; i < 97; ++i) {
            char c = distr(rng);
            w.Write(std::string_view{&c, 1});
            data.push_back(c);
        }
    }

    close(fds[1]);
    char buf[97];
    r = read(fds[0], buf, sizeof(buf));
    REQUIRE(r == sizeof(buf));
    REQUIRE(g.writes_count >= sizeof(buf) / kBufCapacity);
    CHECK(std::string_view{buf, sizeof(buf)} == data);

    close(fds[0]);
}

TEST_CASE("BigWrites") {
    PCGRandom rng{43};

    int fds[2];
    int r = pipe(fds);
    REQUIRE(r != -1);
    WriteGlitch g{fds[1], true};

    std::string data;
    {
        UniformCharDistribution distr('a', 'z');
        BufWriter w(fds[1], 5);
        for (size_t i = 0; i < 250; ++i) {
            size_t l = rng() & 15;
            std::string s(l, ' ');
            std::generate(s.begin(), s.end(), [&] { return distr(rng); });
            int r = w.Write(s);
            REQUIRE(r >= 0);
            data += s;
        }
    }

    close(fds[1]);
    std::string data2(data.size(), ' ');
    r = read(fds[0], data2.data(), data2.size());
    CHECK(r == static_cast<int>(data.size()));
    CHECK(data == data2);
}
