
#ifdef USE_REX

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __GNUC__
/* gcc doesn't know _Thread_local from C11 yet */
#  define thread_local __thread
#elif __STDC_VERSION__ >= 201112L
#  define thread_local _Thread_local
#elif defined(_MSC_VER)
#  define thread_local __declspec( thread )
#else
#  error Cannot define thread_local
#endif

extern thread_local int rexnet_last_error;
extern thread_local int rexnet_last_error_close;

#if !defined(PLATFORM_WINDOWS)
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <sys/ioctl.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#else
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

int sock_capture_error();

const char* sock_error_string(int err);

bool sock_has_error(int err);

bool rexnet_socket_close(int fd);
bool rexnet_socket_create(int family, int type, int protocol, int &fd);
bool rexnet_bind(int fd, struct sockaddr *addr, socklen_t addr_size);
bool rexnet_listen(int fd, int backlog = 4);
bool rexnet_accept(int fd, int &fd_accepted);
bool rexnet_recv(int fd, char *buf, int offs, int size, ssize_t &n);
bool rexnet_send(int fd, char *buf, int offs, int size, ssize_t &n);

#endif
