#include <long-jump.hpp>

int Main(int, char**, char**) {
    JumpBuf buf;
    if (SetJump(&buf) > 0) {
        return 0;
    }
    LongJump(&buf, 123);
}
