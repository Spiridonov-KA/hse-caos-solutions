#include "count-substrings.h"

#include <build.hpp>
#include <distributions.hpp>
#include <pcg-random.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

std::ostream& operator<<(std::ostream& os, SubstringsCount cnt) {
    return os << "{rop = " << cnt.rop_num << ", os = " << cnt.os_num << "}";
}

TEST_CASE("JustWorks") {
    CHECK(CountSubstrings("learn_os_and_use_rop_for_good") ==
          SubstringsCount{.rop_num = 1, .os_num = 1});
    CHECK(CountSubstrings("roposros") ==
          SubstringsCount{.rop_num = 1, .os_num = 2});
    CHECK(CountSubstrings("osososroproprop") ==
          SubstringsCount{.rop_num = 3, .os_num = 3});
}

std::string GenerateBiasedString(size_t length, PCGRandom& rng) {
    constexpr const char* interesting_substrings[] = {"o",  "s",   "os",
                                                      "ro", "rop", "r"};
    std::string result;
    result.reserve(length + 2);
    UniformCharDistribution distr('a', 'z');

    while (result.length() < length) {
        if (rng.Generate32() % 3 != 0) {
            result.push_back(distr(rng));
        } else {
            const char* substring =
                interesting_substrings[rng.Generate32() %
                                       (sizeof(interesting_substrings) /
                                        sizeof(*interesting_substrings))];
            result += substring;
        }
    }

    return result;
}

SubstringsCount DummyCountSubstrings(std::string_view s) {
    std::string::size_type start_pos = 0;
    SubstringsCount res;
    {
        while ((start_pos = s.find("os", start_pos)) != std::string::npos) {
            ++res.os_num;
            ++start_pos;
        }
    }

    {
        start_pos = 0;
        while ((start_pos = s.find("rop", start_pos)) != std::string::npos) {
            ++res.rop_num;
            ++start_pos;
        }
    }
    return res;
}

TEST_CASE("StressTest") {
    PCGRandom rng{42};
    rng.Warmup();

    size_t len = 1;
    for (size_t i = 1; i < 6; ++i) {
        len *= 10;
        for (size_t j = 0; j < 10; ++j) {
            std::string test = GenerateBiasedString(len, rng);
            CHECK(DummyCountSubstrings(test) == CountSubstrings(test.c_str()));
        }
    }

    if constexpr (kBuildType == BuildType::Release) {
        for (size_t i = 6; i < 8; ++i) {
            len *= 10;
            std::string test = GenerateBiasedString(len, rng);
            CHECK(DummyCountSubstrings(test) == CountSubstrings(test.c_str()));
        }
    }
}
