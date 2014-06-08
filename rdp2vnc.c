#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "rdp.h"
#include "session.h"
#include "vnc.h"

int g_process = 1;

void
signal_handler(int signal)
{
	if (signal == SIGINT) {
		g_process = 0;
	}
}

static void
process_connection(int client_fd, const char *server_ip, uint16_t server_port)
{
	int server_fd;
	struct sockaddr_in server_addr;
	r2v_session_t *session = NULL;

	/* connect to VNC server */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		r2v_log_error("create socket for VNC server error: %s", ERRMSG);
		goto fail;
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1) {
		r2v_log_error("convert ip '%s' error: %s", server_ip, ERRMSG);
		goto fail;
	}
	if (connect(server_fd, (struct sockaddr *)&server_addr,
				sizeof(server_addr)) == -1) {
		r2v_log_error("connect to VNC server error: %s", ERRMSG);
		goto fail;
	}

	/* init session */
	session = r2v_session_init(client_fd, server_fd);
	if (session == NULL) {
		r2v_log_error("session init failed");
		goto fail;
	}

	/* start proxy */
	r2v_session_transmit(session);

fail:
	r2v_session_destory(session);
}

static void
parse_address(const char *address, char *ip, int len, uint16_t *port)
{
	char *colon;

	if (address == NULL) {
		return;
	}

	colon = strchr(address, ':');
	if (colon) {
		/* if ip buffer is larger than ip in address, then copy it,
		 * otherwise ignore it */
		if (colon - address <= len) {
			memcpy(ip, address, colon - address);
		}
		if (*(colon + 1) != '\0') {
			*port = atoi(colon + 1);
		}
	} else {
		/* if ip buffer is larger than ip in address, then copy it,
		 * otherwise ignore it */
		if (strlen(address) <= len) {
			memcpy(ip, address, strlen(address));
		}
	}
}

static void
usage()
{
}

int
main(int argc, char *argv[])
{
	int optval = 1;
	struct sigaction act;

	int listen_fd, client_fd;
	char listen_ip[INET_ADDRSTRLEN], server_ip[INET_ADDRSTRLEN];
	uint16_t listen_port, server_port;
	struct sockaddr_in listen_addr;

	/* parse command line arguments */
	int opt;
	struct option opts[] = {
		{"listen", required_argument, NULL, 'l'},
		{"server", required_argument, NULL, 's'},
		{"password", required_argument, NULL, 'p'},
		{"help", no_argument, NULL, 'h'}
	};
	char *listen_address = NULL;
	char *server_address = NULL;
	while ((opt = getopt_long(argc, argv, "l:s:p:h", opts, NULL)) != -1) {
		switch (opt) {
		case 'l':
			listen_address = optarg;
			break;
		case 's':
			server_address = optarg;
			break;
		case 'h':
		case '?':
		default:
			break;
		}
	}
	/* default listen address is 0.0.0.0:3389 */
	strcpy(listen_ip, "0.0.0.0");
	listen_port = 3389;
	parse_address(listen_address, listen_ip, sizeof(listen_ip), &listen_port);
	/* server address has no default value */
	server_ip[0] = '\0';
	server_port = 0;
	parse_address(server_address, server_ip, sizeof(server_ip), &server_port);
	if (server_ip[0] == '\0' || server_port == 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	/* set signal handler */
	memset(&act, 0, sizeof(act));
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGINT, &act, NULL) < 0) {
		r2v_log_error("register signal handler error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		r2v_log_error("create listen socket error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	/* set address reuse */
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))
		== -1) {
		r2v_log_error("set socket reuse address error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(listen_port);
	if (inet_pton(AF_INET, listen_ip, &listen_addr.sin_addr) != 1) {
		r2v_log_error("convert ip '%s' error: %s", listen_ip, ERRMSG);
		exit(EXIT_FAILURE);
	}

	if (bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr))
		== -1) {
		r2v_log_error("bind socket to address error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	if (listen(listen_fd, 64) == -1) {
		r2v_log_error("listen socket error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	r2v_log_info("listening new connection");
	while (g_process) {
		client_fd = accept(listen_fd, NULL, NULL);
		if (client_fd == -1) {
			r2v_log_error("accept new connection error: %s", ERRMSG);
			continue;
		}
		process_connection(client_fd, server_ip, server_port);
	}
	close(listen_fd);

	return 0;
}
