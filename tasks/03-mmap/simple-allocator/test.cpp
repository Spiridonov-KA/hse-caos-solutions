#include "simple-allocator.hpp"

#include <algorithms.hpp>
#include <assert.hpp>
#include <pcg-random.hpp>
#include <strings.hpp>
#include <syscalls.hpp>

constexpr size_t kBlockSize = 16;

void Use(void* ptr, PCGRandom& rng) {
    auto values = reinterpret_cast<uint32_t*>(ptr);
    for (size_t i = 0; i < kBlockSize / sizeof(*values); ++i) {
        values[i] = rng();
    }
}

#define ASSERT_ALLOC(ptr, rng)                                                 \
    do {                                                                       \
        ASSERT(ptr != nullptr, "Allocation failed");                           \
        Use(ptr, rng);                                                         \
    } while (false)

void TestSimple(PCGRandom& rng) {
    void* obj1 = Allocate16();
    ASSERT_ALLOC(obj1, rng);
    void* obj2 = Allocate16();
    ASSERT_ALLOC(obj2, rng);

    Deallocate16(obj1);
    Deallocate16(obj2);
}

void TestReused(PCGRandom& rng) {
    void* original_obj = Allocate16();
    ASSERT_ALLOC(original_obj, rng);
    Deallocate16(original_obj);

    constexpr size_t kBufSize = 16;
    void* buf[kBufSize];
    size_t allocated = 0;
    bool found = false;

    while (allocated < kBufSize && !found) {
        void* obj = buf[allocated++] = Allocate16();
        ASSERT_ALLOC(obj, rng);
        found = obj == original_obj;
    }

    ASSERT(found, "Allocations are not reused");
    for (size_t i = 0; i < allocated; ++i) {
        Deallocate16(buf[i]);
    }
}

template <size_t N, size_t M>
void TestRepeatedRandomDeallocations(size_t iterations, PCGRandom& rng) {
    static_assert(M < N);

    void* pool[N];
    for (size_t i = 0; i < M; ++i) {
        pool[i] = Allocate16();
        ASSERT_ALLOC(pool[i], rng);
    }

    for (size_t it = 0; it < iterations; ++it) {
        for (size_t j = M; j < N; ++j) {
            pool[j] = Allocate16();
            ASSERT_ALLOC(pool[j], rng);
        }

        Shuffle(pool, pool + N, rng);

        for (size_t j = M; j < N; ++j) {
            Deallocate16(pool[j]);
        }
    }

    for (size_t i = 0; i < M; ++i) {
        Deallocate16(pool[i]);
    }
}

extern "C" [[noreturn]] void Main() {
    PCGRandom rng{321};

    RUN_TEST(TestSimple, rng);
    RUN_TEST(TestReused, rng);

    RUN_TEST((TestRepeatedRandomDeallocations<32, 0>), 1000, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<32, 16>), 1000, rng);

    RUN_TEST((TestRepeatedRandomDeallocations<2048, 0>), 1000, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<2048, 1024>), 1000, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<2048, 1536>), 1000, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<2048, 1792>), 1000, rng);

    Exit(0);
}
