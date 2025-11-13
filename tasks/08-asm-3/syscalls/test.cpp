#include "syscalls.hpp"

#include <checksum.hpp>
#include <mm.hpp>
#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fcntl.h>
#include <sys/mman.h>

TEST_CASE("Pipe") {
    PCGRandom rng{Catch::getSeed()};
    int fds[2];
    {
        int ret = Pipe2(fds, 0);
        CHECK(ret == 0);
    }

    constexpr size_t kBufSize = 4096;
    char buf[kBufSize];
    std::generate(buf, buf + kBufSize, [&rng]() { return rng(); });
    OrderedCheckSum original;
    original.Append(buf);

    {
        auto ret = Write(fds[1], buf, kBufSize);
        CHECK(ret == kBufSize);
    }

    char buf2[kBufSize];
    {
        auto ret = Read(fds[0], buf2, kBufSize);
        CHECK(ret == kBufSize);
    }
    OrderedCheckSum read;
    read.Append(buf2);
    CHECK(original.GetDigest() == read.GetDigest());

    Close(fds[0]);
    Close(fds[1]);
}

constexpr const char* kFileName = "/tmp/deleteme-syscalls";

TEST_CASE("Open") {
    PCGRandom rng{Catch::getSeed()};
    int fd = Open(kFileName, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    CHECK(fd > 0);

    constexpr size_t kBufSize = 4096;
    char buf[kBufSize];
    std::generate(buf, buf + kBufSize, [&rng]() { return rng(); });
    OrderedCheckSum original;
    original.Append(buf);

    {
        auto written = Write(fd, buf, sizeof(buf));
        CHECK(written == kBufSize);
    }

    Close(fd);

    fd = Open(kFileName, O_RDONLY, 0);
    CHECK(fd > 0);
    char buf2[kBufSize];
    {
        auto read = Read(fd, buf2, sizeof(buf2));
        CHECK(read == kBufSize);
    }

    OrderedCheckSum read;
    read.Append(buf2);
    CHECK(original.GetDigest() == read.GetDigest());

    Close(fd);
}

TEST_CASE("MMap") {
    PCGRandom rng{Catch::getSeed()};
    int fd = Open(kFileName, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    CHECK(fd > 0);

    constexpr size_t kBufSize = 4096;
    char buf[kBufSize];

    OrderedCheckSum original;
    for (size_t i = 0; i < 4; ++i) {
        original.Reset();
        std::generate(buf, buf + kBufSize, [&rng]() { return rng(); });
        original.Append(buf);
        int written = Write(fd, buf, kBufSize);
        CHECK(written == kBufSize);
    }

    Close(fd);

    fd = Open(kFileName, O_RDONLY, 0);
    CHECK(fd > 0);
    void* ptr =
        MMap(nullptr, kBufSize, PROT_READ, MAP_PRIVATE, fd, 3 * kBufSize);
    REQUIRE(reinterpret_cast<uint64_t>(ptr) <= static_cast<uint64_t>(-4096));
    char* mem = reinterpret_cast<char*>(ptr);
    OrderedCheckSum mapped;
    mapped.Append(std::span{mem, mem + kBufSize});
    CHECK(original.GetDigest() == mapped.GetDigest());

    bool is_valid_before = IsValidPage(ptr);
    int ret = MUnMap(ptr, kBufSize);
    bool is_valid_after = IsValidPage(ptr);

    CHECK(is_valid_before);
    CHECK(ret == 0);
    CHECK(!is_valid_after);
}
