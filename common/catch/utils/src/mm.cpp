#include <mm.hpp>

#include <internal-assert.hpp>

#include <cerrno>
#include <sys/mman.h>

bool IsValidPage(void* page) {
    unsigned char vec[1];
    int result = mincore(page, kPageSize, vec);
    if (result != 0) {
        int err = errno;
        INTERNAL_ASSERT(err == ENOMEM);
    }
    return result == 0;
}
