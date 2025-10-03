#include <glitch.hpp>

#include <utility>  // IWYU pragma: keep (used in macro expansion)

#define XX(name, cname, ret, ...)                                              \
    name##Hook* name##Impl = nullptr;                                          \
                                                                               \
    extern "C" ret __real_##cname(ARGS_MAP(ARG_DECL, __VA_ARGS__));            \
                                                                               \
    ret Real##name(ARGS_MAP(ARG_DECL, __VA_ARGS__)) {                          \
        return __real_##cname(ARGS_MAP(ARG_NAME, __VA_ARGS__));                \
    }                                                                          \
                                                                               \
    extern "C" ret __wrap_##cname(ARGS_MAP(ARG_DECL, __VA_ARGS__)) {           \
        if (auto hook = name##Impl) {                                          \
            return hook->name(ARGS_MAP(ARG_NAME, __VA_ARGS__));                \
        }                                                                      \
        return Real##name(ARGS_MAP(ARG_NAME, __VA_ARGS__));                    \
    }                                                                          \
                                                                               \
    name##Guard::name##Guard() : prev_hook_(std::exchange(name##Impl, this)) { \
    }                                                                          \
                                                                               \
    name##Guard::~name##Guard() {                                              \
        name##Impl = prev_hook_;                                               \
    }

#define GLITCH_INTERNALS
#include <glitch-calls.inc>
#undef GLITCH_INTERNALS

#undef XX
