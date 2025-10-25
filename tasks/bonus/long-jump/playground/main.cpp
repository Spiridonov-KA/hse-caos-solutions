#include <long-jump.hpp>

#include <cstddef>
#include <iostream>

void DoStep(JumpBuf* buf, size_t& n) {
    if (n == 1) {
        return;
    }

    if (n % 2 == 0) {
        n /= 2;
    } else {
        (n *= 3) += 1;
    }
    LongJump(buf, 1);
}

int main() {
    size_t n;
    std::cin >> n;

    JumpBuf buf;
    if (SetJump(&buf) > 0) {
        std::cout << n << "\n";
        DoStep(&buf, n);
        return 0;
    }

    DoStep(&buf, n);
}
