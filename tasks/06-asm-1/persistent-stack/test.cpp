#include "stack.hpp"

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pcg-random.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

static_assert(sizeof(Node) == 24);
static_assert(offsetof(Node, next) == 0);
static_assert(offsetof(Node, reference_count) == 8);
static_assert(offsetof(Node, value) == 16);

struct NodePtr {
    NodePtr() = default;

    NodePtr(const NodePtr& other) : node_(other.node_) {
        if (*this) {
            IncRef(node_);
        }
    }

    NodePtr& operator=(const NodePtr& other) {
        NodePtr tmp{other};
        Swap(tmp);
        return *this;
    }

    NodePtr(NodePtr&& other) : NodePtr() {
        Swap(other);
    }

    NodePtr& operator=(NodePtr&& other) {
        NodePtr tmp{std::move(other)};
        Swap(tmp);
        return *this;
    }

    ~NodePtr() {
        Reset();
    }

    void Swap(NodePtr& other) {
        std::swap(node_, other.node_);
    }

    Node* GetRaw() const {
        return node_;
    }

    explicit operator bool() const {
        return node_ != nullptr;
    }

    void Reset() {
        if (!*this) {
            return;
        }
        DecRef(std::exchange(node_, nullptr));
    }

    static NodePtr FromRaw(Node* node) {
        return NodePtr{node};
    }

  private:
    NodePtr(Node* node) : node_(node) {
    }

    Node* node_{nullptr};
};

NodePtr Push(const NodePtr& head, uint64_t value) {
    return NodePtr::FromRaw(Push(head.GetRaw(), value));
}

TEST_CASE("Simple") {
    Node* node1 = Push(nullptr, 1);
    REQUIRE(node1 != nullptr);
    CHECK(node1->value == 1);
    CHECK(node1->reference_count == 1);
    CHECK(node1->next == nullptr);

    auto node2 = Push(node1, 38);
    REQUIRE(node2 != nullptr);
    DecRef(node1);

    CHECK(node2->value == 38);
    CHECK(node2->next == node1);
    CHECK(node2->reference_count == 1);

    auto node3 = Push(node2, 321);
    REQUIRE(node3 != nullptr);
    DecRef(node2);

    CHECK(node3->value == 321);
    CHECK(node3->next == node2);
    CHECK(node3->reference_count == 1);

    CHECK(node1->value == 1);
    CHECK(node1->reference_count == 1);
    CHECK(node1->next == nullptr);

    DecRef(node3);
}

TEST_CASE("Paths") {
    constexpr size_t kMaxNodes = 150;
    constexpr size_t kMinNodes = 100;

    PCGRandom rng{Catch::getSeed()};

    struct WrappedNode {
        NodePtr node;
        Node* next;
        uint64_t value;
    };

    // Macro preserves callsite
#define CHECK_NODES(range)                                                     \
    do {                                                                       \
        for (auto& node : range) {                                             \
            if (!node.node) {                                                  \
                continue;                                                      \
            }                                                                  \
            auto rnode = node.node.GetRaw();                                   \
            CHECK(rnode->next == node.next);                                   \
            CHECK(rnode->value == node.value);                                 \
        }                                                                      \
    } while (false)

    std::vector<WrappedNode> nodes;
    nodes.push_back(WrappedNode{
        .node = NodePtr(),
        .next = nullptr,
        .value = 0,
    });

    for (size_t i = 0; i < 30'000; ++i) {
        size_t idx = rng() % nodes.size();
        if (rng() % 128 == 0) {
            nodes.push_back(nodes[idx]);
            continue;
        }
        auto value = rng.Generate64();
        auto next = nodes[idx].node.GetRaw();
        auto node = Push(nodes[idx].node, value);
        nodes.push_back(WrappedNode{
            .node = std::move(node),
            .next = next,
            .value = value,
        });

        if (nodes.size() < kMaxNodes) {
            continue;
        }

        std::ranges::shuffle(nodes, rng);
        CHECK_NODES(std::span{nodes}.subspan(kMinNodes));
        nodes.resize(kMinNodes);
    }

    CHECK_NODES(nodes);
    nodes.clear();

#undef CHECK_NODES
}
