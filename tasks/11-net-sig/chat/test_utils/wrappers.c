#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>

#ifdef SOCKET_CLEANUP
static const char *sock_path = NULL;

static void rm_unix_sock() {
    if (sock_path) {
        unlink(sock_path);
    }
}

void __real__exit(int status);
void __wrap__exit(int status) {
    rm_unix_sock();
    __real__exit(status);
}
#endif

static int unix_sock = -1;
static int tcp_sock = -1;

#ifdef __cplusplus
extern "C" {
#endif
int __real_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int __wrap_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (addr->sa_family == AF_INET6) {
        if (getenv("TCP_BIND_FAIL")) {
            errno = EADDRINUSE;
            return -1;
        }
        tcp_sock = sockfd;
    } else if (addr->sa_family == AF_UNIX) {
        if (getenv("UNIX_BIND_FAIL")) {
            errno = EADDRINUSE;
            return -1;
        }
        unix_sock = sockfd;
    }
    return __real_bind(sockfd, addr, addrlen);
}

// need both checks. bind() may return success even if socket is already in use.
int __real_listen(int sockfd, int backlog);
int __wrap_listen(int sockfd, int backlog) {
    if ((sockfd == tcp_sock && getenv("TCP_LISTEN_FAIL"))
            || (sockfd == unix_sock && getenv("UNIX_LISTEN_FAIL"))) {
        errno = EADDRINUSE;
        return -1;
    }
    return __real_listen(sockfd, backlog);

}

#ifdef __cplusplus
}
#endif

#ifdef SOCKET_CLEANUP
int __real_main(int argc, char **argv, char **envp);
int __wrap_main(int argc, char **argv, char **envp) {
    assert(argc == 3);
    sock_path = argv[2];
    atexit(rm_unix_sock);  // for manual testing
    return __real_main(argc, argv, envp);
}
#endif
