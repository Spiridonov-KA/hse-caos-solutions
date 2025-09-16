#include "writer.hpp"

#include "wrappers.hpp"

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <random>

inline void InitRng() {
    srand(Catch::getSeed());
}

TEST_CASE("JustWorks") {
    int fds[2];
    int ret = pipe(fds);
    REQUIRE(ret != -1);
    TargetFd = fds[1];

    {
        BufWriter w(fds[1], 100);
        w.Write("aba");
        w.Write("caba");
        w.Write("daba");
    }

    close(fds[1]);

    char data[11];
    int r = read(fds[0], data, sizeof(data));
    REQUIRE(r == 11);
    REQUIRE(std::string_view{data, sizeof(data)} == "abacabadaba");
    close(fds[0]);
}

TEST_CASE("CorrectNumberOfWrites") {
    int fds[2];
    int ret = pipe(fds);
    REQUIRE(ret != -1);
    TargetFd = fds[1];

    {
        WritesCount = 0;

        BufWriter w(fds[1], 3);
        w.Write("ab");
        w.Write("abc");  // +1
        w.Write("abc");  // +1
        w.Write("abc");  // +1
        w.Flush();       // +1
        REQUIRE(WritesCount == 4);
        w.Flush();
        REQUIRE(WritesCount == 4);
    }

    {
        WritesCount = 0;

        BufWriter w{fds[1], 1000};
        w.Write("aba");
        w.Write("naba");
        w.Write("123");
        REQUIRE(WritesCount == 0);
        w.Flush();
        REQUIRE(WritesCount == 1);
    }

    close(fds[1]);

    char buf[21];
    int r = read(fds[0], buf, sizeof(buf));
    REQUIRE(r == 21);
    REQUIRE(std::string_view{buf, sizeof(buf)} == "ababcabcabcabanaba123");

    close(fds[0]);
}

TEST_CASE("ErrorPropagation") {
    int fds[2];
    int ret = pipe(fds);
    REQUIRE(ret != -1);
    TargetFd = fds[1];
    WritesCount = 0;

    SECTION("Write") {
        NextError = EIO;
        BufWriter w(fds[1], 3);
        int wr = w.Write("");
        REQUIRE(wr >= 0);
        wr = w.Write("a");
        REQUIRE(wr >= 0);
        CHECK(WritesCount == 0);

        wr = w.Write("cab");
        CHECK(wr == -EIO);
    }

    SECTION("Flush") {
        NextError = ENOSPC;

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
    EnableTrim = true;
    InitRng();
    std::mt19937 rng{Catch::getSeed()};

    int fds[2];
    int r = pipe(fds);
    REQUIRE(r != -1);
    TargetFd = fds[1];

    std::string data;
    {
        BufWriter w(fds[1], 5);

        std::uniform_int_distribution<char> distr('a', 'z');
        for (size_t i = 0; i < 97; ++i) {
            char c = distr(rng);
            w.Write(std::string_view{&c, 1});
            data.push_back(c);
        }
    }

    close(fds[1]);
    char buf[97];
    r = read(fds[0], buf, sizeof(buf));
    REQUIRE(r == 97);
    CHECK(std::string_view{buf, sizeof(buf)} == data);

    EnableTrim = false;
    close(fds[0]);
}

TEST_CASE("BigWrites") {
    EnableTrim = true;
    InitRng();
    std::mt19937 rng{Catch::getSeed()};

    int fds[2];
    int r = pipe(fds);
    REQUIRE(r != -1);
    TargetFd = fds[1];

    std::string data;
    {
        std::uniform_int_distribution<char> distr('a', 'z');
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

    EnableTrim = false;
}
