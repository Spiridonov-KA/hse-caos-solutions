#include <glitch.hpp>

#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

struct MainGlitch final : MainGuard {
    virtual int Main(int argc, char** argv, char** env) override {
        assert(argc == 4);
        char* prefill_str = getenv("PREFILL_SIZE");
        if (prefill_str) {
            long prefill_size = strtol(prefill_str, NULL, 10);
            int fd = open(argv[1], O_RDWR | O_CREAT, 0644);

            assert(fd >= 0);
            int truncate_ret = ftruncate(fd, prefill_size);
            assert(truncate_ret == 0);
            (void)truncate_ret;

            close(fd);
        }
        return RealMain(argc, argv, env);
    }
};

static MainGlitch main_glitch;
