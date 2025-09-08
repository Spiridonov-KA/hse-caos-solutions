#include "reverse-list.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <span>
#include <utility>

inline void FreeList(ListNode* node) {
    while (node != nullptr) {
        delete std::exchange(node, node->next);
    }
}

void CheckReverseOn(std::span<ListNode> nodes) {
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i].value = static_cast<int>(i);
        if (i + 1 < nodes.size()) {
            nodes[i].next = &nodes[i + 1];
        } else {
            nodes[i].next = nullptr;
        }
    }

    auto reversed = Reverse(&nodes[0]);
    CHECK(reversed == &nodes.back());
    ListNode* prev = nullptr;
    for (size_t i = 0; i < nodes.size(); ++i) {
        CHECK(nodes[i].value == static_cast<int>(i));
        CHECK(nodes[i].next == prev);
        prev = &nodes[i];
    }
}

template <size_t N>
void CheckReverse() {
    std::array<ListNode, N> nodes;
    CheckReverseOn(nodes);
}

TEST_CASE("Works") {
    CHECK(Reverse(nullptr) == nullptr);

    {
        auto node = new ListNode{
            .value = 1,
            .next = nullptr,
        };

        auto reversed = Reverse(node);
        CHECK(reversed == node);
        CHECK(node->value == 1);
        CHECK(node->next == nullptr);

        node->next = new ListNode{
            .value = 3,
            .next = nullptr,
        };
        auto tail = node->next;
        reversed = Reverse(node);

        CHECK(reversed == tail);
        CHECK(reversed->next == node);

        FreeList(reversed);
    }

    SECTION("Reverse-2") {
        CheckReverse<2>();
    }

    SECTION("Reverse-3") {
        CheckReverse<3>();
    }

    SECTION("Reverse-4") {
        CheckReverse<4>();
    }

    SECTION("Reverse-5") {
        CheckReverse<5>();
    }
}

TEST_CASE("BigList") {
    std::vector<ListNode> nodes(15'000);
    CheckReverseOn(nodes);
}
