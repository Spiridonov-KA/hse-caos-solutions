#include <catch2/catch_test_macros.hpp>

extern uint64_t (*realH)();
extern uint64_t (*realG)();

extern "C" uint64_t GWrapper();
extern "C" uint64_t HWrapper();
extern "C" uint64_t IndirectCallWrapper(uint64_t (*f)(uint64_t, uint64_t),
                                        uint64_t (*g)(), uint64_t (*h)());

TEST_CASE("Simple") {
    realG = []() -> uint64_t { return 456; };
    realH = []() -> uint64_t { return 123; };

    uint64_t result = IndirectCallWrapper(
        [](uint64_t a, uint64_t b) { return a + b; }, GWrapper, HWrapper);

    REQUIRE(result == 123 + 456);

    result = IndirectCallWrapper([](uint64_t a, uint64_t b) { return a & ~b; },
                                 HWrapper, GWrapper);
    REQUIRE(result == (123 & ~456));
}

extern "C" uint64_t ReadRSP();
extern "C" uint64_t ReadRSP2(uint64_t, uint64_t);
asm(R"(
ReadRSP:
ReadRSP2:
    mov rax, rsp
    ret
)");

TEST_CASE("StackAlignment") {
    uint64_t result1 = IndirectCallWrapper(
        [](uint64_t a, uint64_t b) { return ((a & 15) << 4) | (b & 15); },
        ReadRSP, ReadRSP);

    {
        INFO("Stack of g or h is not aligned");
        REQUIRE(result1 == (128 | 8));
    }

    uint64_t result2 = IndirectCallWrapper(
        ReadRSP2, []() -> uint64_t { return 0; },
        []() -> uint64_t { return 456; });
    {
        INFO("Stack of f is not aligned");
        REQUIRE((result2 & 15) == 8);
    }
}

TEST_CASE("BigNumbers") {
    constexpr uint64_t kValue1 = 0x0123456789ABCDEFull;
    constexpr uint64_t kValue2 = 0xF543861BECD7A209ull;

    uint64_t result =
        IndirectCallWrapper([](uint64_t a, uint64_t b) { return ~a & b; },
                            []() { return kValue1; }, []() { return kValue2; });

    REQUIRE(result == (~kValue1 & kValue2));
}
