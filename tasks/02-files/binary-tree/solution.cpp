#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stack>
#include <unistd.h>

struct Node {
    int32_t key;
    int32_t left_idx;
    int32_t right_idx;
};

int OpenFile(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        std::cout << "file open failed: " << errno << " - "
                  << std::strerror(errno) << std::endl;
    }
    return fd;
}

constexpr size_t sizeOfNode = sizeof(Node);

Node ReadWholeNode(int fd, off_t offset) {
    char buffer[sizeOfNode];
    ssize_t totalRead = 0;
    while (totalRead < static_cast<ssize_t>(sizeOfNode)) {
        ssize_t numBytes = pread(fd, buffer + totalRead, sizeOfNode - totalRead,
                                 offset + totalRead);
        if (numBytes == -1) {
            std::cerr << "Read error: " << std::strerror(errno) << std::endl;
            exit(1);
        }
        if (numBytes == 0) {
            std::cerr << "Unexpected EOF" << std::endl;
            exit(1);
        }
        totalRead += numBytes;
    }
    Node node;
    memcpy(&node, buffer, sizeOfNode);
    return node;
}

void PrintTreeInOrder(int fd) {
    std::stack<Node> tree;
    tree.push(ReadWholeNode(fd, 0));
    while (!tree.empty()) {
        auto& curNode = tree.top();
        if (curNode.right_idx > 0) {
            tree.push(ReadWholeNode(fd, sizeOfNode * curNode.right_idx));
            curNode.right_idx = -1;
            continue;
        }
        auto left_idx = curNode.left_idx;
        std::cout << curNode.key << ' ';
        tree.pop();
        if (left_idx != 0) {
            tree.push(ReadWholeNode(fd, sizeOfNode * left_idx));
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect input format\n"
                     "Usage: ./a.out bin.dat";
        return 1;
    }
    int fd = OpenFile(argv[1]);
    PrintTreeInOrder(fd);
    close(fd);
}
