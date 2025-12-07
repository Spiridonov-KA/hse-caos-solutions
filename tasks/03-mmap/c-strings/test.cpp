#include "c-strings.hpp"

#include <benchmark/run.hpp>
#include <bit>
#include <defer.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

TEST_CASE("NonAlloc") {
    SECTION("StrLen") {
        CHECK(StrLen("") == 0);
        CHECK(StrLen("aba") == 3);
        CHECK(StrNLen("", 10) == 0);
        CHECK(StrNLen("", 0) == 0);
        CHECK(StrNLen("aba", 10) == 3);
        CHECK(StrNLen("aba", 2) == 2);

        std::string text(1 << 17, 'b');
        auto c_text = text.c_str();
        auto times = RunWithWarmup(
            [c_text] {
                return StrNLen(c_text, 10);
            },
            1, 20000);
        CHECK(times.cpu_time < 100ms);
    }

    SECTION("StrCmp") {
        CHECK(StrCmp("", "aba") < 0);
        CHECK(StrCmp("", "") == 0);
        CHECK(StrCmp("aba", "abac") < 0);
        CHECK(StrCmp("aba", "aba") == 0);

        {
            auto a = "asdf";
            auto b = "asdfghjk";
            for (size_t i = 0; i < 10; ++i) {
                if (i <= 4) {
                    CHECK(StrNCmp(a, b, i) == 0);
                } else {
                    CHECK(StrNCmp(a, b, i) < 0);
                    CHECK(StrNCmp(b, a, i) > 0);
                }
            }
        }

        {
            std::string a = "qwerty";
            std::string b = "qwerty";
            for (size_t i = 0; i < 10; ++i) {
                INFO("i = " << i);
                CHECK(StrNCmp(a.c_str(), b.c_str(), i) == 0);
            }
        }
    }

    SECTION("StrCat") {
        char buf[10] = "";
        char* s;

        s = StrCat(buf, "aba");
        CHECK(s == buf);
        CHECK(StrCmp(buf, "aba") == 0);

        s = StrCat(buf, "caba");
        CHECK(s == buf);
        CHECK(StrCmp(buf, "abacaba") == 0);

        s = StrCat(buf, "da");
        CHECK(s == buf);
        CHECK(StrCmp(buf, "abacabada") == 0);
    }

    SECTION("StrStr") {
        const char* haystack;

        haystack = "";
        CHECK(StrStr(haystack, "") == haystack);
        CHECK(StrStr(haystack, "daba") == nullptr);

        haystack = "abacaba";
        CHECK(StrStr(haystack, "") == haystack);
        CHECK(StrStr(haystack, "cab") == haystack + 3);
        CHECK(StrStr(haystack, haystack) == haystack);
        CHECK(StrStr(haystack, "daba") == nullptr);
        CHECK(StrStr(haystack, "aaaaaaaaaaa") == nullptr);
    }

    SECTION("StrStrBig") {
        std::string haystack;
        for (size_t i = 1; i < (1 << 20); ++i) {
            haystack.push_back('a' + std::countr_zero(i));
        }
        auto c_haystack = haystack.c_str();

        CHECK(StrStr(c_haystack, "g") == c_haystack + 63);
        CHECK(StrStr(c_haystack, "z") == nullptr);
        CHECK(StrStr(c_haystack, "acabadab") == c_haystack + 2);

        auto needle = haystack.substr(3);
        CHECK(StrStr(c_haystack, needle.c_str()) == c_haystack + 3);

        needle.push_back('w');
        CHECK(StrStr(c_haystack, needle.c_str()) == nullptr);
    }
}

TEST_CASE("StrDup") {
    // Preserves callsite
#define CHECK_STR(s)                                                           \
    do {                                                                       \
        auto s2 = StrDup(s);                                                   \
        Defer cleanup([s2] {                                                   \
            Deallocate(s2);                                                    \
        });                                                                    \
        CHECK(StrCmp(s2, s) == 0);                                             \
    } while (false)

    CHECK_STR("");
    CHECK_STR("aba");

    std::string big(1 << 17, 'a');
    CHECK_STR(big.c_str());

#undef CHECK_STR

#define CHECK_STR(s, n)                                                        \
    do {                                                                       \
        auto ln = static_cast<size_t>(n);                                      \
        auto s2 = StrNDup(s, ln);                                              \
        Defer cleanup([s2] {                                                   \
            Deallocate(s2);                                                    \
        });                                                                    \
        CHECK(StrLen(s2) == std::min(StrLen(s), ln));                          \
        CHECK(StrNCmp(s2, s, ln) == 0);                                        \
    } while (false)

    CHECK_STR("", 10);
    CHECK_STR("", 0);
    CHECK_STR("test", 2);
    CHECK_STR("test", 4);
    CHECK_STR("test", 1000000000);

#undef CHECK_STR

    SECTION("Performance") {
        static constexpr size_t kCopies = 16384;
        std::vector<char*> duplicates;
        duplicates.reserve(kCopies);
        Defer cleanup([&duplicates] {
            for (auto s : duplicates) {
                Deallocate(s);
            }
        });

        std::string text(1 << 17, 'a');
        auto times = Run([&] {
            auto c_text = text.c_str();
            for (size_t i = 0; i < kCopies; ++i) {
                auto duplicate = StrNDup(c_text, 10);
                duplicates.push_back(duplicate);
                REQUIRE(StrLen(duplicate) == 10);
                REQUIRE(StrNCmp(duplicate, c_text, 10) == 0);
            }
            return c_text;
        });
        CHECK(times.cpu_time < 1s);
    }
}

TEST_CASE("AStrCat") {
#define CHECK_STRS(s1, s2)                                                     \
    do {                                                                       \
        auto concat = AStrCat(s1, s2);                                         \
        Defer cleanup([concat] {                                               \
            Deallocate(concat);                                                \
        });                                                                    \
        auto correct = std::string{s1} + std::string{s2};                      \
        CHECK(StrCmp(concat, correct.c_str()) == 0);                           \
    } while (false)

    CHECK_STRS("", "");
    CHECK_STRS("", "non empty");
    CHECK_STRS("non empty", "");
    CHECK_STRS("weird \x7f", "string \xff");

    {
        std::string big1(1 << 17, 'a');
        std::string big2(1 << 17, 'b');
        auto big1_raw = big1.c_str();
        auto big2_raw = big2.c_str();
        CHECK_STRS(big1_raw, big2_raw);
    }

#undef CHECK_STRS
}
