#include "tree-sum.hpp"

#include <build.hpp>
#include <pcg-random.hpp>

#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <numeric>

static_assert(offsetof(TreeNode, value) == 0);
static_assert(offsetof(TreeNode, left) == 8);
static_assert(offsetof(TreeNode, right) == 16);

TEST_CASE("Simple") {
    TreeNode a{.value = 123, .left = nullptr, .right = nullptr};
    TreeNode b{.value = 456, .left = nullptr, .right = nullptr};
    TreeNode c{.value = 789, .left = &a, .right = &b};

    CHECK(TreeSum(nullptr) == 0);
    CHECK(TreeSum(&b) == 456);
    CHECK(TreeSum(&c) == 123 + 456 + 789);
}

static constexpr size_t kNodes =
    kBuildType == BuildType::Release ? 100'000 : 10'000;

TEST_CASE("Chain") {
    PCGRandom rng{Catch::getSeed()};

    std::vector<TreeNode> nodes(kNodes, TreeNode{0, nullptr, nullptr});

    uint64_t sum = 0;
    for (size_t i = 0; i + 1 < kNodes; ++i) {
        if (rng() & 1) {
            nodes[i].left = &nodes[i + 1];
        } else {
            nodes[i].right = &nodes[i + 1];
        }
        nodes[i].value = rng.Generate64();
        sum += nodes[i].value;
    }

    CHECK(TreeSum(&nodes[0]) == sum);
}

TEST_CASE("Random") {
    PCGRandom rng{Catch::getSeed(), 321};

    std::vector<TreeNode> nodes(kNodes);
    std::vector<size_t> indices;
    indices.reserve(kNodes);

    auto add_to = [&rng](auto add_to, TreeNode* root, TreeNode* node) {
        TreeNode** child;
        if (rng() & 1) {
            child = &root->left;
        } else {
            child = &root->right;
        }

        if (!*child) {
            *child = node;
            return;
        }
        add_to(add_to, *child, node);
    };

    for (size_t i = 0; i < 10; ++i) {
        indices.resize(kNodes);
        std::ranges::iota(indices, 0);

        uint64_t sum = 0;
        for (auto& node : nodes) {
            node = TreeNode{rng.Generate64(), nullptr, nullptr};
            sum += node.value;
        }

        while (indices.size() > 1) {
            std::swap(indices[rng() % indices.size()], indices.back());
            auto node_idx = indices.back();
            indices.pop_back();

            auto root_idx = indices[rng() % indices.size()];
            add_to(add_to, &nodes[root_idx], &nodes[node_idx]);
        }

        CHECK(TreeSum(&nodes[indices[0]]) == sum);
    }
}
