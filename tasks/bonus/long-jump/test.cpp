#include <long-jump.hpp>

#include <overload.hpp>
#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#define UNREACHABLE FAIL("Should be unreachable")

TEST_CASE("Simple") {
    JumpBuf buf;
    if (SetJump(&buf) > 0) {
        return;
    }

    LongJump(&buf, 1);
    UNREACHABLE;
}

TEST_CASE("HeapAlloc") {
    auto buf = new JumpBuf;

    if (SetJump(buf) == 321) {
        delete buf;
        return;
    }

    LongJump(buf, 321);
    UNREACHABLE;
}

static void ChainBody(std::vector<int>* log) {
    JumpBuf buf1;
    if (SetJump(&buf1) > 0) {
        log->push_back(0);
        return;
    }

    JumpBuf buf2;
    if (SetJump(&buf2) > 0) {
        log->push_back(1);
        LongJump(&buf1, 1);
        UNREACHABLE;
    }

    JumpBuf buf3;
    if (SetJump(&buf3) > 0) {
        log->push_back(2);
        LongJump(&buf2, 1);
        UNREACHABLE;
    }

    log->push_back(3);
    LongJump(&buf3, 1);
    UNREACHABLE;
}

TEST_CASE("Chain") {
    std::vector<int> log;
    ChainBody(&log);
    CHECK(log == std::vector<int>{3, 2, 1, 0});
}

TEST_CASE("ReturnValue") {
    constexpr uint64_t kMagicValue = 0xfee1deadbeef;

    JumpBuf buf;
    if (SetJump(&buf) == kMagicValue) {
        return;
    }

    LongJump(&buf, kMagicValue);
    UNREACHABLE;
}

struct Error {
    enum Value : uint64_t {
        kDivideByZero = 1,
        kOverflow = 2,
    };
};

using Output = std::variant<std::vector<int64_t>, Error::Value>;

std::ostream& operator<<(std::ostream& out, const Output& result) {
    std::visit(Overload{
                   [&out](const std::vector<int64_t>& v) {
                       out << "{";
                       bool is_first = true;
                       for (auto x : v) {
                           if (!std::exchange(is_first, false)) {
                               out << ", ";
                           }
                           out << x;
                       }
                       out << "}";
                   },
                   [&out](Error::Value e) {
                       switch (e) {
                       case Error::kOverflow:
                           out << "overflow";
                           break;
                       case Error::kDivideByZero:
                           out << "divide by 0";
                       }
                   },
               },
               result);
    return out;
}

static int SafeDiv(int64_t num, int64_t denom, JumpBuf* err_handler) {
    if (denom == 0) {
        LongJump(err_handler, Error::kDivideByZero);
    }
    if (num == std::numeric_limits<int64_t>::min() && denom == -1) {
        LongJump(err_handler, Error::kOverflow);
    }

    return num / denom;
}

static void SafeDivMany(std::span<const int64_t> nums,
                        std::span<const int64_t> denoms, std::span<int64_t> out,
                        JumpBuf* err_handler) {
    for (size_t i = 0; i < nums.size(); ++i) {
        out[i] = SafeDiv(nums[i], denoms[i], err_handler);
    }
}

[[nodiscard]] static uint64_t
SafeDivManyWrapper(std::span<const int64_t> nums,
                   std::span<const int64_t> denoms, std::vector<int64_t>* out) {
    JumpBuf err_handler;
    switch (SetJump(&err_handler)) {
    case 0:
        break;
    case Error::kDivideByZero:
        return Error::kDivideByZero;
    case Error::kOverflow:
        return Error::kOverflow;
    }

    SafeDivMany(nums, denoms, *out, &err_handler);
    return 0;
}

static Output SafeDivMany(std::span<const int64_t> nums,
                          std::span<const int64_t> denoms) {
    std::vector<int64_t> results(nums.size());
    int err = SafeDivManyWrapper(nums, denoms, &results);
    if (err != 0) {
        return Error::Value(err);
    }
    return results;
}

TEST_CASE("NotExceptions") {

    CHECK(SafeDivMany({}, {}) == Output{std::vector<int64_t>{}});
    CHECK(SafeDivMany({{10, 11, 12}}, {{3, 4, 5}}) ==
          Output{std::vector<int64_t>{3, 2, 2}});
    CHECK(SafeDivMany({{1, 2, 3}}, {{0, 1, 2}}) ==
          Output{Error::kDivideByZero});
    CHECK(SafeDivMany({{std::numeric_limits<int64_t>::min()}}, {{-1}}) ==
          Output{Error::kOverflow});

    CHECK(SafeDivMany({{std::numeric_limits<int64_t>::min(),
                        std::numeric_limits<int64_t>::min()}},
                      {{-1, 0}}) == Output{Error::kOverflow});
}
