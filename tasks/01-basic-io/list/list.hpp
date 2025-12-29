#pragma once

#include <cassert>
#include <utility>

class List {
  public:
    // Non-copyable
    List(const List&) = delete;
    List& operator=(const List&) = delete;

    List(List&& other) : List() {
        Swap(other);
    }

    List& operator=(List&& other) {
        List tmp{std::move(other)};
        Swap(tmp);
        return *this;
    }

    List() {
    }

    ~List() {
        Clear();
    }

    void PushBack(int value) {
        if (tail == nullptr) {
            CreateFirstNode(value);
            return;
        }
        auto newTail = new Node{value, tail, nullptr};
        tail->setRightPtr(newTail);
        tail = newTail;
    }

    void PushFront(int value) {
        if (tail == nullptr) {
            CreateFirstNode(value);
            return;
        }
        auto newHead = new Node{value, nullptr, head};
        head->setLeftPtr(newHead);
        head = newHead;
    }

    void PopBack() {
        auto newTail = tail->getLeftPtr();
        if (newTail != nullptr) {
            newTail->setRightPtr(nullptr);
        } else {
            head = nullptr;
        }
        delete tail;
        tail = newTail;
    }

    void PopFront() {
        auto newHead = head->getRightPtr();
        if (newHead != nullptr) {
            newHead->setLeftPtr(nullptr);
        } else {
            tail = nullptr;
        }
        delete head;
        head = newHead;
    }

    int& Back() {
        return tail->getValue();
    }

    int& Front() {
        return head->getValue();
    }

    bool IsEmpty() const {
        return head == nullptr;
    }

    void Swap(List& other) {
        std::swap(head, other.head);
        std::swap(tail, other.tail);
    }

    void Clear() {
        if (IsEmpty()) {
            return;
        }
        auto curNode = head;
        auto nextNode = curNode->getRightPtr();
        while (curNode != nullptr) {
            nextNode = curNode->getRightPtr();
            delete curNode;
            curNode = nextNode;
        }
    }

    // https://en.cppreference.com/w/cpp/container/list/splice
    // Expected behavior:
    // l1 = {1, 2, 3};
    // l1.Splice({4, 5, 6});
    // l1 == {1, 2, 3, 4, 5, 6};
    void Splice(List& other) {
        if (other.IsEmpty()) {
            return;
        }
        if (IsEmpty()) {
            head = other.head;
            tail = other.tail;
        } else {
            tail->setRightPtr(other.head);
            other.head->setLeftPtr(tail);
            tail = other.tail;
        }
        other.head = nullptr;
        other.tail = nullptr;
    }

    template <class F>
    void ForEachElement(F&& f) const {
        if (IsEmpty()) {
            return;
        }
        auto curNode = head;
        auto nextNode = curNode->getRightPtr();
        while (curNode != nullptr) {
            nextNode = curNode->getRightPtr();
            f(curNode->getValue());
            curNode = nextNode;
        }
    }

  private:
    class Node {
        Node* left = nullptr;
        Node* right = nullptr;  // use unique_ptr
        int value;

      public:
        Node(int value) : value{value} {
        }

        Node(int value, Node* left, Node* right)
            : left{left}, right{right}, value{value} {
        }

        void setLeftPtr(Node* ptr) {
            left = ptr;
        }

        void setRightPtr(Node* ptr) {
            right = ptr;
        }

        void setValue(int newValue) {
            value = newValue;
        }

        Node* getLeftPtr() {
            return left;
        }

        Node* getRightPtr() {
            return right;
        }

        int& getValue() {
            return value;
        }
    };

    Node* head = nullptr;
    Node* tail = nullptr;

    void CreateFirstNode(int value) {
        assert(head == nullptr && tail == nullptr);
        head = tail = new Node{value};
    }
};
