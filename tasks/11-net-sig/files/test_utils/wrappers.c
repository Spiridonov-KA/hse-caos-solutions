#define _GNU_SOURCE
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int __real_sigsuspend(const sigset_t* mask);
int __real_pause(void);
int __real_sigprocmask(int how, const sigset_t* restrict set,
                       sigset_t* restrict oldset);

int __real_main(int argc, char** argv, char** envp);

// ----- sigprocmask/sigsuspend delay (check if signals are properly blocked)
// -----

static int spm_first_call = 1;
static int ss_first_call = 1;
static int pause_first_call = 1;
static int need_delay = -1;

static void delay_if_needed(int* first_call) {
    if (need_delay == -1) {
        need_delay = (getenv("DELAY_SIGSUSPEND") != NULL);
    }
    if (need_delay && *first_call) {
        usleep(20000);
        *first_call = 0;
    }
}

int __wrap_sigprocmask(int how, const sigset_t* restrict set,
                       sigset_t* restrict oldset) {
    // print pid -> set mask: signal may be missed
    delay_if_needed(&spm_first_call);
    return __real_sigprocmask(how, set, oldset);
}

int __wrap_sigsuspend(const sigset_t* mask) {
    // if signals are not blocked, usleep will be interrupted and real
    // sigsuspend will miss the signal
    delay_if_needed(&ss_first_call);
    return __real_sigsuspend(mask);
}

int __wrap_pause(void) {
    // same as sigsuspend
    delay_if_needed(&pause_first_call);
    return __real_pause();
}

// PE handling

static void die(__attribute__((unused)) int signum) {
    exit(0);
}

int __wrap_main(int argc, char** argv, char** envp) {
    sigaction(SIGRTMAX, &(struct sigaction){.sa_handler = die}, NULL);
    return __real_main(argc, argv, envp);
}
