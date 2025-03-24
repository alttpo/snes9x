
#ifdef USE_REX

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "rexnet.h"

thread_local int rexnet_last_error = 0;
thread_local int rexnet_last_error_close = 0;

#if !defined(PLATFORM_WINDOWS)
#  define SEND_BUF_CAST(t) ((const void *)(t))
#  define RECV_BUF_CAST(t) ((void *)(t))

const char* sock_error_string(int err) {
	return strerror(err);
}

int sock_capture_error() {
	return errno;
}

bool sock_has_error(int err) {
	return err != 0;
}
#else
#  define SEND_BUF_CAST(t) ((const char *)(t))
#  define RECV_BUF_CAST(t) ((char *)(t))

thread_local char errmsg[256];

const char* sock_error_string(int errcode) {
	DWORD len = FormatMessageA(
        FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errcode,
        0,
        errmsg,
        255,
        nullptr
	);
	if (len != 0)
		errmsg[len] = 0;
	else
		sprintf(errmsg, "error %d", errcode);
	return errmsg;
}

int sock_capture_error() {
	return WSAGetLastError();
}

bool sock_has_error(int err) {
	return FAILED(err);
}
#endif

bool rexnet_socket_close(int fd) {
	if (fd < 0) return false;

	int rc;
#if !defined(PLATFORM_WINDOWS)
	rc = ::close(fd);
#else
	rc = ::closesocket(fd);
#endif
	rexnet_last_error_close = 0;
	if (rc < 0) {
		rexnet_last_error_close = sock_capture_error();
		return false;
	}

	return true;
}

bool rexnet_socket_create(int family, int type, int protocol, int &fd) {
	// create the socket:
#if !defined(PLATFORM_WINDOWS)
	fd = ::socket(family, type, protocol);
#else
	fd = ::WSASocket(family, type, protocol, nullptr, 0, 0);
#endif
	rexnet_last_error = 0;
	if (fd < 0) {
		rexnet_last_error = sock_capture_error();
		return false;
	}

	// enable reuse address:
	int rc = 0;
	int yes = 1;
#if !defined(PLATFORM_WINDOWS)
	rc = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
#else
	rc = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(int));
#endif
	rexnet_last_error = 0;
	if (rc < 0) {
		rexnet_last_error = sock_capture_error();
		goto closeNoError;
	}

	// enable TCP_NODELAY for SOCK_STREAM type sockets:
	if (type == SOCK_STREAM) {
#if !defined(PLATFORM_WINDOWS)
		rc = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int));
#else
		rc = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&yes, sizeof(int));
#endif
		rexnet_last_error = 0;
		if (rc < 0) {
			rexnet_last_error = sock_capture_error();
			goto closeNoError;
		}
	}

	// set non-blocking:
#if !defined(PLATFORM_WINDOWS)
	rc = ioctl(fd, FIONBIO, &yes);
#else
	rc = ioctlsocket(fd, FIONBIO, (u_long *)&yes);
#endif
	rexnet_last_error = 0;
	if (rc < 0) {
		rexnet_last_error = sock_capture_error();
		goto closeNoError;
	}

	return true;

closeNoError:
	rexnet_socket_close(fd);
	fd = -1;
	return false;
}

bool rexnet_bind(int fd, struct sockaddr *addr, socklen_t addr_size) {
	if (fd < 0) return false;

	int rc = ::bind(fd, addr, addr_size);
	rexnet_last_error = 0;
	if (rc < 0) {
		rexnet_last_error = sock_capture_error();
		return false;
	}

	return true;
}

bool rexnet_listen(int fd, int backlog) {
	if (fd < 0) return false;

	int rc = ::listen(fd, backlog);
	rexnet_last_error = 0;
	if (rc < 0) {
		rexnet_last_error = sock_capture_error();
		return false;
	}

	return true;
}

bool rexnet_accept(int fd, int &fd_accepted) {
	if (fd < 0) return false;

	// non-blocking sockets don't require a poll() because accept() handles non-blocking scenario itself.

	// accept incoming connection, discard client address:
	int rc = ::accept(fd, nullptr, nullptr);
	rexnet_last_error = 0;
	if (rc < 0) {
		rexnet_last_error = sock_capture_error();
		return false;
	}

	fd_accepted = rc;

	return true;
}

bool rexnet_recv(int fd, char *buf, int offs, int size, ssize_t &n) {
	if (fd < 0) return false;

#if !defined(PLATFORM_WINDOWS)
	ssize_t rc = ::recv(
		fd,
		RECV_BUF_CAST(buf + offs),
		size,
		0
	);
#else
	ssize_t rc = ::recv(
		fd,
		RECV_BUF_CAST(buf + offs),
		size,
		0
	);
#endif

	rexnet_last_error = 0;
	if (rc < 0) {
		rexnet_last_error = sock_capture_error();
		return false;
	}

	n = rc;
	return true;
}

bool rexnet_send(int fd, char *buf, int offs, int size, ssize_t &n) {
	if (fd < 0) return false;

#if !defined(PLATFORM_WINDOWS)
	ssize_t rc = ::send(
		fd,
		SEND_BUF_CAST(buf + offs),
		size,
		0
	);
#else
	ssize_t rc = ::send(
		fd,
		SEND_BUF_CAST(buf + offs),
		size,
		0
	);
#endif

	rexnet_last_error = 0;
	if (rc < 0) {
		rexnet_last_error = sock_capture_error();
		return false;
	}

	n = rc;
	return true;
}

#endif
