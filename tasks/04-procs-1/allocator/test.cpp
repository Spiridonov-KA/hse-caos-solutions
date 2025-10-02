#include "allocator.hpp"

#include "vector.hpp"

#include <algorithms.hpp>
#include <assert.hpp>
#include <defer.hpp>
#include <io.hpp>
#include <pcg_random.hpp>

#include <utility>

#define ASSERT_ALLOC(ptr, size, rng)                                           \
    do {                                                                       \
        ASSERT(ptr != nullptr);                                                \
        for (size_t i = 0; i < size; ++i) {                                    \
            reinterpret_cast<char*>(ptr)[i] = rng();                           \
        }                                                                      \
        ASSERT(reinterpret_cast<uintptr_t>(ptr) % 16 == 0,                     \
               "Block is not aligned properly");                               \
    } while (false)

void TestWorks(PCGRandom& rng) {
#define _ALLOCATE(ptr, size)                                                   \
    void* ptr = Allocate(size);                                                \
    DEFER {                                                                    \
        Deallocate(ptr);                                                       \
    };                                                                         \
    ASSERT_ALLOC(ptr, size, rng)

#define ALLOCATE(size) _ALLOCATE(UNIQUE_ID(ptr), size)

    ALLOCATE(1);
    ALLOCATE(15);
    ALLOCATE(123);
    ALLOCATE(1024);
    ALLOCATE(4095);
    ALLOCATE(8000);
    ALLOCATE(10000);

#undef ALLOCATE
#undef _ALLOCATE
}

struct ListNode {
    ListNode* next;
};

static_assert(sizeof(ListNode) == 8);

ListNode* TrimHead(ListNode** head) {
    return std::exchange(*head, std::exchange((*head)->next, nullptr));
}

void PushHead(ListNode** where, ListNode* node) {
    node->next = std::exchange(*where, node);
}

ListNode* Shuffle(ListNode* head, PCGRandom& rng) {
    ListNode* lhs = nullptr;
    ListNode* rhs = nullptr;

    while (head != nullptr) {
        PushHead(&lhs, TrimHead(&head));
        std::swap(lhs, rhs);
    }
    if (lhs == nullptr) {
        return rhs;
    }

    lhs = Shuffle(lhs, rng);
    rhs = Shuffle(rhs, rng);
    while (lhs != nullptr || rhs != nullptr) {
        ListNode** victim;
        if (lhs == nullptr || (rhs != nullptr && (rng() & 1))) {
            victim = &rhs;
        } else {
            victim = &lhs;
        }

        PushHead(&head, TrimHead(victim));
    }

    return head;
}

template <class F>
void TestRepeatedAllocations(F&& size_sampler, size_t iterations,
                             size_t allocations, PCGRandom& rng) {
    for (size_t i = 0; i < iterations; ++i) {
        ListNode* head = nullptr;

        for (size_t j = 0; j < allocations; ++j) {
            auto size = size_sampler();
            auto ptr = Allocate(size);
            ASSERT_ALLOC(ptr, size, rng);
            PushHead(&head, reinterpret_cast<ListNode*>(ptr));
        }

        head = Shuffle(head, rng);
        while (head != nullptr) {
            Deallocate(TrimHead(&head));
        }
    }
}

void TestRandomBigAllocations(PCGRandom& rng) {
    auto size_sampler = [&rng]() {
        constexpr size_t kMinAllocSize = 8;
        constexpr size_t kMaxAllocSize = 1 << 14;
        return rng() % (kMaxAllocSize - kMinAllocSize) + kMinAllocSize;
    };
    TestRepeatedAllocations(size_sampler, 100, 1000, rng);
}

void TestRandomSmallAllocations(PCGRandom& rng) {
    auto size_sampler = [&rng]() {
        constexpr size_t kMinAllocSize = 8;
        constexpr size_t kMaxAllocSize = 128;
        return rng() % (kMaxAllocSize - kMinAllocSize) + kMinAllocSize;
    };
    return TestRepeatedAllocations(size_sampler, 5, 250'000, rng);
}

void TestRepeatedAllocations(PCGRandom& rng) {
    static constexpr size_t kPoolSize = 1024;
    static constexpr size_t kMaxAllocationSize = 8192;
    void* pool[kPoolSize];

    for (size_t i = 0; i < 256; ++i) {
        for (size_t j = 0; j < kPoolSize; ++j) {
            auto size = rng() % kMaxAllocationSize;
            auto ptr = Allocate(size);
            ASSERT_ALLOC(ptr, size, rng);
            pool[j] = ptr;
        }

        Shuffle(pool, pool + kPoolSize, rng);

        for (size_t j = 0; j < kPoolSize; ++j) {
            Deallocate(pool[j]);
        }
    }
}

struct TreeNode {
    static constexpr uint64_t kCanaryValue = 7739134852254543268;

    uint64_t head_canary = kCanaryValue;
    Vector<TreeNode> children;
    uint64_t tail_canary = kCanaryValue;

    ~TreeNode() {
        ASSERT(head_canary == kCanaryValue);
        ASSERT(tail_canary == kCanaryValue);
    }
};

TreeNode Merge(TreeNode lhs, TreeNode rhs) {
    for (auto&& x : rhs.children) {
        lhs.children.PushBack(std::move(x));
    }

    return lhs;
}

template <size_t MinChildren, size_t MaxChildren, size_t LargeChildChance>
struct TreeGenerator {
    PCGRandom& rng;
    size_t large_cnt_remaining;

    TreeNode GenTree(size_t depth) {
        TreeNode node;
        if (depth == 0) {
            return node;
        }

        size_t children = rng() % (MaxChildren - MinChildren + 1) + MinChildren;
        node.children.Reserve(children);

        for (size_t i = 0; i < children; ++i) {
            node.children.PushBack(GenTree(depth - 1));
        }

        if (rng() % LargeChildChance == 0 && large_cnt_remaining > 0) {
            --large_cnt_remaining;
            node.children.Resize(2048);
            Shuffle(node.children.begin(), node.children.end(), rng);
        }

        return node;
    }
};

void DeleteTree(TreeNode node, PCGRandom& rng) {
    Shuffle(node.children.begin(), node.children.end(), rng);
    for (auto&& child : node.children) {
        DeleteTree(std::move(child), rng);
    }
}

void TrimRoot(TreeNode& root, size_t max_size, PCGRandom& rng) {
    if (root.children.Size() <= max_size) {
        return;
    }
    Shuffle(root.children.begin(), root.children.end(), rng);
    root.children.Resize(max_size);
}

void TestTree(PCGRandom& rng) {
    for (size_t i = 0; i < 100; ++i) {
        auto root =
            TreeGenerator<0, 10, 100>{
                .rng = rng,
                .large_cnt_remaining = 10,
            }
                .GenTree(4);

        root = Merge(std::move(root),
                     TreeGenerator<5, 15, 100>{
                         .rng = rng,
                         .large_cnt_remaining = 10,
                     }
                         .GenTree(3));

        TrimRoot(root, 10, rng);

        root = Merge(std::move(root),
                     TreeGenerator<20, 100, 1000>{
                         .rng = rng,
                         .large_cnt_remaining = 10,
                     }
                         .GenTree(2));

        TrimRoot(root, 10, rng);

        root = Merge(std::move(root),
                     TreeGenerator<10, 30, 100>{
                         .rng = rng,
                         .large_cnt_remaining = 10,
                     }
                         .GenTree(2));

        DeleteTree(std::move(root), rng);
    }
}

extern "C" [[noreturn]] void Main() {
    PCGRandom rng{424243};

    RUN_TEST(TestWorks, rng);
    RUN_TEST(TestRandomSmallAllocations, rng);
    RUN_TEST(TestRandomBigAllocations, rng);
    RUN_TEST(TestRepeatedAllocations, rng);
    RUN_TEST(TestTree, rng);

    Exit(0);
}
