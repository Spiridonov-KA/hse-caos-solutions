#pragma once

#include <macros.hpp>
#include <sys/types.h>

#define XX(name, cname, ret, ...)                                              \
    struct name##Hook {                                                        \
        virtual ret name(ARGS_MAP(ARG_DECL, __VA_ARGS__)) = 0;                 \
    };                                                                         \
                                                                               \
    ret Real##name(ARGS_MAP(ARG_DECL, __VA_ARGS__));                           \
    extern name##Hook* name##Impl;                                             \
                                                                               \
    struct name##Guard : name##Hook {                                          \
        name##Guard();                                                         \
        ~name##Guard();                                                        \
                                                                               \
      private:                                                                 \
        name##Hook* prev_hook_;                                                \
    };

#define GLITCH_INTERNALS
#include "glitch-calls.inc"
#undef GLITCH_INTERNALS

#undef XX
