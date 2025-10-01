#define NEED_CORR 1
#define NEED_INFO 1
#define NEED_TGZ 1

#include <nejudge/checker.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum { ACCESS_MODE = 0666 };

int checker_main(int argc, char** argv, testinfo_t* ti) {
    int original_umask = umask(0);

    struct stat stb;
    if (lstat(ti->cmd_argv[2], &stb) < 0) {
        fatal_WA("output file '%s' does not exist", ti->cmd_argv[2]);
    }
    if (!S_ISREG(stb.st_mode)) {
        fatal_WA("output file '%s' is not regular", ti->cmd_argv[2]);
    }
    int mode = stb.st_mode & 0777;
    if (mode != (ACCESS_MODE & ~original_umask) && mode != ACCESS_MODE) {
        fatal_WA("output file '%s' has wrong access mode %04o", ti->cmd_argv[2],
                 mode);
    }

    FILE* fin = fopen(ti->cmd_argv[2], "r");
    if (!fin) {
        fatal_WA("cannot open output file '%s'", ti->cmd_argv[2]);
    }

    FILE* fcorr = fopen(argv[4], "r");
    if (!fcorr) {
        fatal_CF("cannot open correct file '%s", argv[4]);
    }

    char *corr_s = NULL, *user_s = NULL;
    size_t corr_z = 0, user_z = 0;
    checker_read_file_f(fcorr, &corr_s, &corr_z);
    checker_read_file_f(fin, &user_s, &user_z);

    if (corr_z != user_z) {
        fatal_WA("output file length mismatch: corr: %zu, user: %zu", corr_z,
                 user_z);
    }
    if (memcmp(corr_s, user_s, corr_z) != 0) {
        fatal_WA("answers mismatch");
    }

    checker_OK();
}

/*
 * Local variables:
 *  c-basic-offset: 4
 * End:
 */
