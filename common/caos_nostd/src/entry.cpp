#include <syscalls.hpp>

#include <cstdint>

int Main(int argc, char** argv, char** env);

char** Environ;

extern "C" [[noreturn]] void Entry(uintptr_t* args) {
    auto argc = *args;
    char** argv = reinterpret_cast<char**>(args + 1);
    Environ = argv + argc + 1;

    int code = Main(static_cast<int>(argc), argv, Environ);

    Exit(code);
}
