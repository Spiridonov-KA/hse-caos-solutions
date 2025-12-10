#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

FILE* protocol = NULL;

enum {
    RUN_OK = 0,
    RUN_WA = 5,
};

void expect(int fd, const char* str, size_t len) {
    size_t toread = len, consumed = 0;
    char buf[1024];
    while (toread > 0) {
        ssize_t L = read(fd, buf, sizeof(buf));
        if (L <= 0) {
            exit(RUN_WA);
        }
        if ((size_t)L > toread) {
            exit(RUN_WA);
        }
        if (memcmp(str + consumed, buf, L)) {
            exit(RUN_WA);
        }
        fwrite(buf, 1, L, protocol);
        toread -= L;
        consumed += L;
    }
}

int main(int argc, char* argv[]) {
    char* test_data_path = argv[1];
    char* protocol_path = argv[2];
    // char* correct_path = argv[3];
    pid_t pid = atoi(argv[4]);

    FILE* test_data = fopen(test_data_path, "rt");
    protocol = fopen(protocol_path, "wt");

    char* buf = NULL;
    size_t bufcap = 0;
    ssize_t linelen = 0;
    while ((linelen = getline(&buf, &bufcap, test_data)) > 0) {
        if (!strcmp(buf, "TERMINATE\n")) {
            fprintf(stderr, "Sending SIGTERM\n");
            kill(pid, SIGTERM);
            const char* msg = "We communicate on my terms.\n";
            expect(STDIN_FILENO, msg, strlen(msg));
        } else if (!strcmp(buf, "WINTERRUPT\n")) {
            kill(pid, SIGRTMAX);
            usleep(20000);
        } else {
            ssize_t written = write(STDOUT_FILENO, buf, linelen);
            if (written != linelen) {
                return RUN_WA;
            }
            expect(STDIN_FILENO, buf, linelen);
        }
    }
    free(buf);
    close(STDOUT_FILENO);
    char tail[100];
    if (read(STDIN_FILENO, tail, sizeof(tail)) > 0) {
        return RUN_WA;
    }
    return RUN_OK;
}
