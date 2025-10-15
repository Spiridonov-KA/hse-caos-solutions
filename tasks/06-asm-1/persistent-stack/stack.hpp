#include <cstdint>

struct Node {
    Node* next;
    uint64_t reference_count;
    uint64_t value;
};

extern "C" void IncRef(Node* node);
extern "C" void DecRef(Node* node);
extern "C" Node* Push(Node* head, uint64_t value);
