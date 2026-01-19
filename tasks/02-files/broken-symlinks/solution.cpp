#include <cerrno>
#include <iostream>

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void PrintFileInfo(const char* filename) {
    struct stat sb;
    if (stat(filename, &sb) != 0) {
        if (lstat(filename, &sb) == 0) {
            std::cout << filename << " (broken symlink)" << std::endl;
            return;
        }
        if (errno == ENOENT) {
            std::cout << filename << " (missing)" << std::endl;
        } else {
            std::cout << std::strerror(errno) << " WTF!\n";
        }

    } else {
        std::cout << filename << '\n';
    }
}

int main(int argc, const char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        PrintFileInfo(argv[i]);
    }
}
