#include <atomic>
#include <cassert>
#include <dlfcn.h>
#include <iostream>
#include <pthread.h>

extern "C" int pthread_create(pthread_t* __restrict__ t,
                              const pthread_attr_t* __restrict__ attr,
                              void* (*entry)(void*),
                              void* __restrict__ arg) throw();

using pthread_create_t = decltype(&pthread_create);

static pthread_create_t real_pthread_create = nullptr;

static pthread_create_t GetRealPThreadCreate() {
    if (real_pthread_create == nullptr) [[unlikely]] {
        real_pthread_create =
            (pthread_create_t)dlsym(RTLD_NEXT, "pthread_create");
        assert(real_pthread_create != nullptr);
    }
    return real_pthread_create;
}

static std::atomic<uint64_t> cnt{0};

extern "C" int pthread_create(pthread_t* __restrict__ t,
                              const pthread_attr_t* __restrict__ attr,
                              void* (*entry)(void*),
                              void* __restrict__ arg) throw() {
    if (cnt.fetch_add(1) + 1 >= 10) {
        std::cerr << "Too many threads created" << std::endl;
        std::abort();
    }
    return GetRealPThreadCreate()(t, attr, entry, arg);
}
