#include <glitch.hpp>

#include <pcg-random.hpp>
#include <sys/types.h>

struct ForkGlitches final : ForkGuard {
    ForkGlitches() : rng_(42) {
    }

    pid_t Fork() override {
        if (rng_() % 2 == 0) {
            return -1;
        }
        return RealFork();
    }

  private:
    PCGRandom rng_;
};

static ForkGlitches fork_glitches;
