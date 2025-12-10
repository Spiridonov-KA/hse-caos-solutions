#include <nejudge/checker2.h>
#include <nejudge/testinfo.h>

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static int do_kill(int pid, int signum) {
    // try to switch to ejexec user
    static int fast_signal = -1;
    if (fast_signal == -1) {  // init
        fast_signal = 0;
        if (getuid() == 0) {
            fast_signal = 1;
        }
        /*
        if (ejexec_uid >= 0) {
            if (setuid(ejexec_uid) < 0 || seteuid(ejexec_uid) < 0 ||
        setreuid(ejexec_uid, ejexec_uid) < 0) { fprintf(stderr, "cannot switch
        user to ejexec\n"); } else { setgid(ejexec_gid); setregid(ejexec_gid,
        ejexec_gid); gid_t grps[] = { ejexec_gid }; setgroups(1, grps);
                fast_signal = 1;
                fprintf(stderr, "fast signalling is available\n");
            }
        }
        */
    }
    if (fast_signal) {
        int res = kill(pid, signum);
        if (res < 0) {
            fprintf(stderr, "kill: %s\n", strerror(errno));
        }
        return res;
    } else {
        return checker_kill(pid, signum);
    }
}

// If the process is not stopped, ejudge will report WTE instead of PE/WA/etc.
// // CF is reported even if the process doesn't terminate.

static void kill_and_exit(int pid, int status, const char* fmt, ...) {
    do_kill(pid, SIGRTMAX);
    checker_drain();

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(status);
}

/*
  argv[1] - test file
  argv[2] - output file
  argv[3] - correct file
  argv[4] - PID
  argv[5] - info file
 */
int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);

    struct passwd* ppp = getpwnam("ejexec");
    int ejexec_uid = -1;
    int ejexec_gid = -1;
    if (!ppp) {
        fprintf(stderr,
                "no 'ejexec' user, using default signal sending mechanism\n");
    } else {
        fprintf(stderr, "user ejexec: %d, %d\n", ppp->pw_uid, ppp->pw_gid);
        ejexec_uid = ppp->pw_uid;
        ejexec_gid = ppp->pw_gid;
        // debug
        // ejexec_uid = 0;
        // ejexec_gid = 0;
    }
    (void)ejexec_uid;
    (void)ejexec_gid;

    if (argc != 6) {
        fatal_CF("wrong number of arguments: expected 6, but actual %d\n",
                 argc);
    }

    FILE* fin = fopen(argv[1], "r");
    if (!fin) {
        fatal_CF("cannot open input file '%s'", argv[1]);
    }

    FILE* fout = fopen(argv[2], "w");
    if (!fout) {
        fatal_CF("cannot open output file '%s'", argv[2]);
    }

    int other_pid = -1;
    if (checker_stoi(argv[4], 10, &other_pid) < 0 || other_pid <= 0) {
        fatal_CF("invalid process pid '%s'", argv[4]);
    }

    testinfo_t* ti = xcalloc(1, sizeof(*ti));
    int err = testinfo_parse(argv[5], ti, NULL);
    if (err < 0) {
        fatal_CF("Parsing of '%s' failed: %s", argv[4], testinfo_strerror(err));
    }

    int* wpipes = NULL;
    int* rpipes = NULL;
    if (ti->cmd_argc > 0) {
        wpipes = malloc(ti->cmd_argc * sizeof(wpipes[0]));
        memset(wpipes, -1, ti->cmd_argc * sizeof(wpipes[0]));
        rpipes = malloc(ti->cmd_argc * sizeof(rpipes[0]));
        memset(rpipes, -1, ti->cmd_argc * sizeof(rpipes[0]));
    }

    for (int i = 0; i < ti->cmd_argc; ++i) {
        rpipes[i] = open(ti->cmd_argv[i], O_RDONLY | O_NONBLOCK, 0);
        if (rpipes[i] < 0) {
            fatal_CF("Cannot open FIFO '%s' for reading: %s", ti->cmd_argv[i],
                     strerror(errno));
        }
        wpipes[i] = open(ti->cmd_argv[i], O_WRONLY, 0);
        if (wpipes[i] < 0) {
            fatal_CF("Cannot open FIFO '%s' for writing: %s", ti->cmd_argv[i],
                     strerror(errno));
        }
    }

    int user_pid = -1;
    if (scanf("%d", &user_pid) != 1) {
        kill_and_exit(other_pid, RUN_PRESENTATION_ERR,
                      "failed to read pid from program");
    }
    if (user_pid != other_pid) {
        kill_and_exit(other_pid, RUN_WRONG_ANSWER_ERR,
                      "user program reports invalid PID");
    }

    // close the reading end of FIFOs
    for (int i = 0; i < ti->cmd_argc; ++i) {
        if (rpipes[i] >= 0) {
            close(rpipes[i]);
        }
        rpipes[i] = -1;
    }

    // revert to the regular blocking I/O mode
    /*
    for (int i = 0; i < ti->cmd_argc; ++i) {
        fcntl(wpipes[i], F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);
    }
    */

    fprintf(stderr, "user: %d, %d\n", getuid(), getgid());

    int current = -1;
    long long value;
    while (fscanf(fin, "%lld", &value) == 1) {
        char buf[16];
        if (value < -100) {
            int mult = -value - 100;
            if (fscanf(fin, "%lld", &value) != 1) {
                fatal_CF("failed to read value");
            }
            if (current >= 0 && current < ti->cmd_argc &&
                wpipes[current] >= 0) {
                for (int i = 0; i < mult; ++i) {
                    memset(buf, ' ', sizeof(buf));
                    snprintf(buf, sizeof(buf), "%lld", value);
                    buf[strlen(buf)] = ' ';
                    buf[sizeof(buf) - 1] = '\n';
                    int r = write(wpipes[current], buf, sizeof(buf));
                    if (r <= 0) {
                        kill_and_exit(other_pid, RUN_WRONG_ANSWER_ERR,
                                      "write error to FIFO %d: %s", current,
                                      strerror(errno));
                    } else if (r != sizeof(buf)) {
                        fatal_CF("short write to FIFO %d: %s", current,
                                 strerror(errno));
                    }
                }
                usleep(40000);
            }
        } else if (value < 0) {
            int index = ~value;
            current = -1;
            if (index >= 0 && index < ti->cmd_argc) {
                current = index;
                usleep(10000);
                do_kill(other_pid, SIGRTMIN + index);
                // fprintf(stderr, "sending %d\n", index);
                usleep(10000);
            }
        } else if (!value) {
            if (current >= 0 && current < ti->cmd_argc &&
                wpipes[current] >= 0) {
                // fprintf(stderr, "close pipe %d\n", current);
                close(wpipes[current]);
                wpipes[current] = -1;
            }
        } else if (value > 0) {
            if (current >= 0 && current < ti->cmd_argc &&
                wpipes[current] >= 0) {
                memset(buf, ' ', sizeof(buf));
                if (snprintf(buf, sizeof(buf), "%lld", value) >= sizeof(buf)) {
                    abort();
                }
                buf[strlen(buf)] = ' ';
                buf[sizeof(buf) - 1] = '\n';
                // fprintf(stderr, ">>>%s", buf);
                int r = write(wpipes[current], buf, sizeof(buf));
                if (r <= 0) {
                    kill_and_exit(other_pid, RUN_WRONG_ANSWER_ERR,
                                  "write error to FIFO %d: %s", current,
                                  strerror(errno));
                } else if (r != sizeof(buf)) {
                    fatal_CF("short write to FIFO %d: %s", current,
                             strerror(errno));
                }
                // usleep(40000);
            }
        }
    }

    // that's it
    if (getenv("LATE_SIGTERM")) {
        fprintf(stderr, "Delay SIGTERM\n");
        usleep(200000);
    } else {
        usleep(10000);
    }
    do_kill(other_pid, SIGTERM);
    for (int i = 0; i < ti->cmd_argc; ++i) {
        if (wpipes[i] >= 0) {
            close(wpipes[i]);
        }
        if (rpipes[i] >= 0) {
            close(rpipes[i]);
        }
    }
    free(wpipes);
    free(rpipes);

    // copy program output
    fclose(stdout);
    // Skip leading newline
    getchar_unlocked();
    int c;
    while ((c = getchar_unlocked()) != EOF) {
        putc_unlocked(c, fout);
    }
    fclose(fin);
    fclose(fout);
    return 0;
}
