#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>  // int64_t
#include <ios>
#include <limits>
#include <sstream>
#include <string>
#include <unistd.h>  // read/write
#include <vector>

template <typename T>
class FibSequence {
    static inline std::vector<T> sequence_;
    static_assert(std::is_unsigned_v<T> == true,
                  "Type T must be unsigned for FibSequence<T> class");

  public:
    FibSequence() {
        sequence_.reserve(1000);
        sequence_.insert(sequence_.end(), {1, 1, 2});
        for (int i = 3; sequence_[i - 2] < sequence_[i - 1]; ++i) {
            sequence_.push_back(sequence_[i - 1] + sequence_[i - 2]);
        }
        sequence_.back() = std::numeric_limits<T>::max();
    }

    int64_t getFibRoot(T number) {
        if (number == 0) {
            return -1;
        }
        if (number == 1) {
            return 1;
        }
        auto iter =
            std::lower_bound(sequence_.begin(), sequence_.end(), number);
        if (*iter == number) {
            return iter - sequence_.begin();
        }
        return iter - sequence_.begin() - 1;
    }
};

std::string readStdin() {
    int numBytes;
    char buf[BUFSIZ];
    std::string input;
    input.reserve(BUFSIZ);
    while ((numBytes = read(STDIN_FILENO, buf, BUFSIZ)) > 0) {
        input.insert(input.end(), buf, buf + numBytes);
    }
    return input;
}

void writeStdout(const std::string& str) {
    auto totalBytes = str.size();
    ssize_t numBytes = 0;
    size_t writtenBytes = 0;
    while ((numBytes = write(STDOUT_FILENO, str.c_str() + writtenBytes,
                             totalBytes - writtenBytes)) > 0) {
        writtenBytes += numBytes;
    }
}

int main() {
    auto inputData = readStdin();
    std::stringstream in(inputData), out;
    int64_t number;
    in >> std::hex;
    out << std::hex;
    FibSequence<uint64_t> fibNum;
    while (in >> number) {
        out << fibNum.getFibRoot(number) << "\n";
    }
    writeStdout(out.str());
}
