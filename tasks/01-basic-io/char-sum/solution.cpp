#include <algorithm>
#include <cctype>  // std::isdigit
#include <charconv>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <vector>

std::vector<char> readStdin() {
    int numBytes;
    char buf[BUFSIZ];
    std::vector<char> input;
    input.reserve(BUFSIZ);
    while ((numBytes = read(STDIN_FILENO, buf, BUFSIZ)) > 0) {
        input.insert(input.end(), buf, buf + numBytes);
    }
    return input;
}

int sumDigitsInString(const std::vector<char>& input) {
    int result = 0;
    std::for_each(input.begin(), input.end(), [&result](char x) {
        if (std::isdigit(x)) {
            result += x - '0';
        }
    });
    return result;
}

void writeStdout(const std::string& str) {
    auto totalBytes = str.size();
    ssize_t numBytes = 0;
    size_t writtenBytes = 0;
    while ((numBytes = write(STDOUT_FILENO, str.c_str() + writtenBytes,
                             totalBytes - writtenBytes)) > 0) {
        writtenBytes += numBytes;
    }
    if (write(1, "\n", 1) < 0) {
        exit(1);
    }
}

int main() {
    std::vector<char> input = readStdin();
    int result = sumDigitsInString(input);

    char buf[BUFSIZ];
    auto [_, ec] = std::to_chars(buf, buf + BUFSIZ, result);

    if (ec != std::errc{}) {
        writeStdout(std::make_error_code(ec).message());
    } else {
        writeStdout(buf);
    }
}
