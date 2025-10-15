#pragma once

#include <cstdint>

struct TreeNode {
    uint64_t value;
    TreeNode* left;
    TreeNode* right;
};

extern "C" uint64_t TreeSum(TreeNode* node);
