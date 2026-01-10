#include "reverse-list.hpp"

ListNode* Reverse(ListNode* node) {
    if (node == nullptr || node->next == nullptr) {
        return node;
    }
    ListNode* nextNode = nullptr;
    ListNode* curNode = node;
    ListNode* prevNode = nullptr;
    while (curNode != nullptr) {
        nextNode = curNode->next;
        curNode->next = prevNode;
        prevNode = curNode;
        curNode = nextNode;
    }
    return prevNode;
}
