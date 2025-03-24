
#ifdef USE_REX

#include <thread>
#include "snes9x.h"
#include "rexnet.h"

extern "C" {
#include "trex.h"
#include "trex_opcodes.h"
}

#define MAX_CLIENTS 4

struct trex_context ctx;
uint32_t stack[32];
uint8_t  code[8192];
uint32_t locals[MAX_CLIENTS][16];
struct trex_sm machines[MAX_CLIENTS];
struct trex_sh handlers[MAX_CLIENTS][4];
struct trex_sm *machines_lkup[MAX_CLIENTS];

std::thread th_net;
bool rexnet_running = false;

void S9xRexNetThread(void);

void S9xRexInit(void) {
	// initialize trex context to execute state machines with:
	trex_context_init(&ctx, nullptr, stack, 32);

	ctx.machines = machines_lkup;
	ctx.machines_count = 1;

	machines[0].handlers_count = 1;
	handlers[0][0].pc_start = &code[0];

	code[0] = PSH1;
	code[1] = 0;

	for (int i = 0; i < 4; i++) {
		machines_lkup[i] = &machines[i];
		trex_sm_init(
			&ctx,
			&machines[i],
			0,             // iterations
			handlers[i],   // handlers
			0,             // handlers_count
			0,             // syscalls
			0,             // syscalls_count
			locals[i],     // locals
			16             // locals_count
		);
	}

	th_net = std::thread(S9xRexNetThread);
}

// on network thread:
void S9xRexNetThread(void) {
	int server_fd;
	int client_fds[MAX_CLIENTS];
	struct timeval timeout;
	fd_set readfds;
	
	if (!rexnet_socket_create(AF_INET, SOCK_STREAM, 0, server_fd)) {
		S9xMessage(S9X_ERROR, 0, sock_error_string(rexnet_last_error));
		return;
	}
	
	struct sockaddr_in listen_addr;
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(8979);
	
	if (!rexnet_bind(server_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr))) {
		S9xMessage(S9X_ERROR, 0, sock_error_string(rexnet_last_error));
		return;
	}
	
	if (!rexnet_listen(server_fd, MAX_CLIENTS)) {
		S9xMessage(S9X_ERROR, 0, sock_error_string(rexnet_last_error));
		return;
	}
	
	timeout.tv_sec = 1000;
	timeout.tv_usec = 0;
	
	rexnet_running = true;
	while (rexnet_running) {
		// Clear the socket set
		FD_ZERO(&readfds);
		
		// Add server socket to set
		FD_SET(server_fd, &readfds);
		int max_sd = server_fd;
		
		// Add client sockets to set
		for (int i = 0; i < MAX_CLIENTS; i++) {
			int sd = client_fds[i];
			if (sd > 0)
				FD_SET(sd, &readfds);
			if (sd > max_sd)
				max_sd = sd;
		}
		
		// Wait for activity on one of the sockets, timeout is NULL, so wait indefinitely
		int rc = select(max_sd + 1, &readfds, nullptr, nullptr, &timeout);
		if (rc == 0) {
			continue;
		}
		if (rc < 0) {
			// TODO: report select error?
			continue;
		}
		
		// If something happened on the master socket, then it's an incoming connection
		if (FD_ISSET(server_fd, &readfds)) {
			int sd;
			struct sockaddr_in address;
			socklen_t addrlen;
			if ((sd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
			
			printf("New connection, socket fd is %d, ip: %s, port: %d\n", sd, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
			
			// Add new socket to array of sockets
			for (int i = 0; i < MAX_CLIENTS; i++) {
				if (client_fds[i] == 0) {
					client_fds[i] = sd;
					break;
				}
			}
		}

		for (int i = 0; i < MAX_CLIENTS; i++) {
			int sd = client_fds[i];
			if (!FD_ISSET(sd, &readfds)) continue;

			ssize_t n;
			if (!rexnet_recv(sd, buf, 0, 1024, n)) {
				rexnet_socket_close(sd);
				client_fds[i] = 0;
				continue;
			}

			// TODO: parse commands + args
			// TODO: one command = syscall by name
		}
	}
}

// on emulator thread:
void S9xRexExec(void) {
	trex_exec(&ctx, 25);
}

#endif
