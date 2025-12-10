#include <nejudge/checker2.h>
#include <nejudge/testinfo.h>

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum { enable_debug = 0 };

/*
  argv[1] - subcommand: start/stop
  argv[2] - test file path
  argv[3] - correct file path
  argv[4] - .inf-file path
 */
int main(int argc, char* argv[]) {
    if (argc != 5) {
        fatal_CF("5 arguments expected, but %d provided", argc);
    }

    if (enable_debug) {
        char buf[4096];
        fprintf(stderr, "initializer works: %s\n", getcwd(buf, sizeof(buf)));
    }

    testinfo_t* ti = xcalloc(1, sizeof(*ti));
    int err = testinfo_parse(argv[4], ti, NULL);
    if (err < 0) {
        fatal_CF("Parsing of '%s' failed: %s", argv[4], testinfo_strerror(err));
    }

    umask(0);

    // the program name is not stored in cmd_argv, only meaningful arguments
    for (int i = 0; i < ti->cmd_argc; ++i) {

        if (strcmp(argv[1], "start") == 0) {
            if (mkfifo(ti->cmd_argv[i], 0666) < 0) {
                fatal_CF("Failed to create a FIFO '%s': %s", ti->cmd_argv[i],
                         strerror(errno));
            }
        } else {
            remove(ti->cmd_argv[i]);
        }

        if (enable_debug) {
            fprintf(stderr, "created FIFO '%s'\n", ti->cmd_argv[i]);
            struct stat statbuf;
            if (stat(ti->cmd_argv[i], &statbuf) < 0) {
                perror("failed to stat the FIFO");
            } else {
                fprintf(stderr, "FIFO has mode: %o\n", statbuf.st_mode);
            }
        }
    }

    return 0;
}
