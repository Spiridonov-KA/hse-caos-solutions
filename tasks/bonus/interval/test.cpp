#include "interval.hpp"

#include <pcg-random.hpp>
#include <unused.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cfenv>
#include <numbers>
#include <utility>
#include <vector>

double Spread(const Interval& v) {
    return v.GetUpper() - v.GetLower();
}

TEST_CASE("JustWorks") {
    for (auto sign : {-1, 1}) {
        INFO("Sign is " << sign);

        Interval v{0};
        v += 1 * sign;
        v += Interval{1.} / (2 * sign);
        v += Interval{1.} / (3 * sign);
        v += Interval{1.} / (5 * sign);
        v += Interval{1.} / (7 * sign);

        double real = 457.0 / (210 * sign);
        CHECK(v.GetLower() <= real);
        CHECK(v.GetUpper() >= real);
        CHECK(Spread(v) <= 1e-10);
    }

    Interval u{-2, 1};
    CHECK(u.GetLower() == -2);
    CHECK(u.GetUpper() == 1);
}

TEST_CASE("Product") {
    {
        Interval v1{0, 1};
        Interval v2{-1, 1};
        auto p1 = v1 * v2;
        CHECK(p1.GetLower() == -1);
        CHECK(p1.GetUpper() == 1);

        auto p2 = v2 * v1;
        CHECK(p2.GetLower() == -1);
        CHECK(p2.GetUpper() == 1);
    }

    {
        Interval v1{2, 3};
        Interval v2{3, 4};

        auto p = v1 * v2;
        CHECK(p.GetLower() == 6);
        CHECK(p.GetUpper() == 12);
    }

    {
        Interval v1{-2, 1};
        Interval v2{-1, 3};

        auto p1 = v1 * v2;
        CHECK(p1.GetLower() == -6);
        CHECK(p1.GetUpper() == 3);

        auto p2 = v2 * v1;
        CHECK(p2.GetLower() == -6);
        CHECK(p2.GetUpper() == 3);
    }
}

TEST_CASE("Fractions") {
    PCGRandom rng{424243};

    std::vector<std::pair<uint32_t, uint32_t>> ratios;
    for (size_t i = 0; i < 10'000; ++i) {
        auto num = rng.Generate32() % 1000;
        auto denom = rng.Generate32() % 1000 + 1;

        ratios.emplace_back(num, denom);
    }

    Interval v{0};
    CHECK(Spread(v) == 0);

    auto iterate = [&] {
        std::ranges::shuffle(ratios, rng);
        for (auto [num, denom] : ratios) {
            v += Interval{double(num)} / double(denom);
        }

        std::ranges::shuffle(ratios, rng);
        for (auto [num, denom] : ratios) {
            v -= Interval{double(num)} / double(denom);
        }
    };
    iterate();

    CHECK(v.GetLower() <= 0);
    CHECK(v.GetUpper() >= 0);
    auto spread_before = Spread(v);
    CHECK(spread_before <= 3e-7);

    iterate();

    CHECK(Spread(v) > spread_before);
}

Interval ArcTan(Interval x) {
    Interval v{0};
    Interval cur{x};

    for (int i = 0; i < 31; ++i) {
        if ((i % 2) == 0) {
            auto delta = cur / (i + 1);
            if ((i % 4) == 0) {
                v += delta;
            } else {
                v -= delta;
            }
        }
        cur *= x;
    }

    return v;
}

TEST_CASE("Constants") {
    SECTION("e") {
        Interval v{0};
        Interval cur{1};
        for (int i = 0; i < 30; ++i) {
            v += cur;
            cur /= i + 1;
        }

        CHECK(cur.GetUpper() > 0);
        CHECK(v.GetLower() <= std::numbers::e);
        CHECK(v.GetUpper() >= std::numbers::e);
        CHECK(Spread(v) <= 1e-10);
    }

    SECTION("pi") {
        Interval v = ArcTan(Interval{1} / 5) * 4 - ArcTan(Interval{1} / 239);
        v *= 4;

        CHECK(v.GetLower() <= std::numbers::pi);
        CHECK(v.GetUpper() >= std::numbers::pi);
        CHECK(Spread(v) <= 1e-10);
    }
}

TEST_CASE("Division") {
    Interval v{-1, 1};

    for (auto denom : {-1, 1}) {
        auto d = v / denom;
        INFO("Denominator is " << denom);
        CHECK(d.GetLower() == -1);
        CHECK(d.GetUpper() == 1);
    }
}

TEST_CASE("Fenv") {
    auto prev = std::fegetround();
    std::fesetround(FE_TOWARDZERO);

    UNUSED(Interval{1} * (Interval{2} - 3 + 4) / 5);
    CHECK(std::fegetround() == FE_TOWARDZERO);

    std::fesetround(FE_UPWARD);
    UNUSED(Interval{1} * (Interval{2} - 3 + 4) / 5);
    CHECK(std::fegetround() == FE_UPWARD);

    std::fesetround(prev);
}
