#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t runproc(char** argv, int fdin, int fdout) {
    pid_t child;
    if (!(child = fork())) {
        dup2(fdin, STDIN_FILENO);
        dup2(fdout, STDOUT_FILENO);
        close(fdin);
        close(fdout);
        execvp(argv[0], argv);
        _exit(2);
    }
    close(fdin);
    close(fdout);
    return child;
}

int main(int argc, char* argv[]) {
    int idx2 = 1;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], ";")) {
            argv[i] = NULL;
            idx2 = i + 1;
            break;
        }
    }
    int pipe0[2], pipe1[2];
    if (pipe2(pipe1, O_CLOEXEC) || pipe2(pipe0, O_CLOEXEC)) {
        perror("pipe");
        return 1;
    }

    // append child PID to interactor args
    pid_t pid = runproc(argv + idx2, pipe0[0], pipe1[1]);
    char buf[100];
    sprintf(buf, "%d", pid);
    argv[idx2 - 1] = buf;
    argv[idx2] = NULL;

    runproc(argv + 1, pipe1[0], pipe0[1]);
    int status = 0;
    int res = 0;
    while (wait(&status) != -1) {
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Interactor failed with exit code %d\n",
                    WEXITSTATUS(status));
            res = 1;
        }
    }
    return res;
}
