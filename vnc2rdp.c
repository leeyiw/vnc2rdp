/**
 * vnc2rdp: proxy for RDP client connect to VNC server
 *
 * Copyright 2014 Yiwei Li <leeyiw@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#define ERROR(...) \
	fprintf(stderr, "ERROR: "); \
	fprintf(stderr, __VA_ARGS__)

int g_process = 1;

void
signal_handler(int signal)
{
	if (signal == SIGINT) {
		g_process = 0;
	}
}

static void
process_connection(int client_fd, const v2r_session_opt_t *opt)
{
	int server_fd;
	struct sockaddr_in server_addr;
	v2r_session_t *session = NULL;

	/* connect to VNC server */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		v2r_log_error("create socket for VNC server error: %s", ERRMSG);
		goto fail;
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(opt->vnc_server_port);
	if (inet_pton(AF_INET, opt->vnc_server_ip, &server_addr.sin_addr) != 1) {
		v2r_log_error("convert ip '%s' error: %s", opt->vnc_server_ip, ERRMSG);
		goto fail;
	}
	if (connect(server_fd, (struct sockaddr *)&server_addr,
				sizeof(server_addr)) == -1) {
		v2r_log_error("connect to VNC server error: %s", ERRMSG);
		goto fail;
	}

	/* init session */
	session = v2r_session_init(client_fd, server_fd, opt);
	if (session == NULL) {
		v2r_log_error("session init failed");
		goto fail;
	}

	/* start proxy */
	v2r_session_transmit(session);

fail:
	v2r_session_destory(session);
}

static void
parse_address(const char *address, char *ip, int len, uint16_t *port)
{
	char *colon;
	size_t ip_len;

	if (address == NULL) {
		return;
	}

	colon = strchr(address, ':');
	if (colon) {
		/* if ip buffer is larger than ip in address, then copy it,
		 * otherwise ignore it */
		ip_len = colon - address;
		if (ip_len != 0 && ip_len <= len) {
			memcpy(ip, address, ip_len);
			ip[ip_len] = '\0';
		}
		if (*(colon + 1) != '\0') {
			*port = atoi(colon + 1);
		}
	} else {
		/* if ip buffer is larger than ip in address, then copy it,
		 * otherwise ignore it */
		ip_len = strlen(address);
		if (ip_len <= len) {
			memcpy(ip, address, ip_len);
			ip[ip_len] = '\0';
		}
	}
}

static void
usage(const char *name)
{
	const char *msg =
"Usage: %s [options] server:port\n"
"\n"
"  -l, --listen=ADDRESS       listen address, default: 0.0.0.0:3389\n"
"  -p, --password=PASSWORD    VNC server password, for VNC authentication\n"
"  -s, --shared               connect to VNC server use share mode, share \n"
"                             desktop with other clients (default)\n"
"  -n, --noshared             connect to VNC server use exclusive mode, \n"
"                             by disconnect all other client\n"
"  -v, --viewonly             disable transfer of mouse and keyboard events \n"
"                             from the client to the server\n"
"  -h, --help                 print this help message and exit\n"
"\n";

	fprintf(stderr, msg, name);
}

int
main(int argc, char *argv[])
{
	char listen_ip[INET_ADDRSTRLEN];
	uint8_t shared = 0, noshared = 0;
	uint16_t listen_port;
	int listen_fd, client_fd, ch, optval = 1;
	struct sockaddr_in listen_addr;
	struct sigaction act;
	struct option longopts[] = {
		{"listen", required_argument, NULL, 'l'},
		{"password", required_argument, NULL, 'p'},
		{"shared", no_argument, NULL, 's'},
		{"noshared", no_argument, NULL, 'n'},
		{"viewonly", no_argument, NULL, 'v'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};
	v2r_session_opt_t opt;

	/* clear session option */
	memset(&opt, 0, sizeof(opt));

	/* default listen address is 0.0.0.0:3389 */
	strcpy(listen_ip, "0.0.0.0");
	listen_port = 3389;

	while ((ch = getopt_long(argc, argv, "l:p:snvh", longopts, NULL)) != -1) {
		switch (ch) {
		case 'l':
			parse_address(optarg, listen_ip, sizeof(listen_ip), &listen_port);
			break;
		case 'p':
			strncpy(opt.vnc_password, optarg, sizeof(opt.vnc_password));
			break;
		case 's':
			shared = 1;
			break;
		case 'n':
			noshared = 1;
			break;
		case 'v':
			opt.viewonly = 1;
			break;
		case 'h':
		case '?':
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	/* --shared and --noshared cannot use at same the time */
	if (shared && noshared) {
		ERROR("Both --shared and --noshared are specified\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	opt.shared = noshared ? 0 : 1;

	/* command line option must contain VNC server address */
	if (argc - optind != 1) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* server address has no default value */
	parse_address(argv[optind], opt.vnc_server_ip, sizeof(opt.vnc_server_ip),
				  &(opt.vnc_server_port));
	if (opt.vnc_server_ip[0] == '\0' || opt.vnc_server_port == 0) {
		ERROR("Bad format of server address: %s\n", argv[optind]);
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* set signal handler */
	memset(&act, 0, sizeof(act));
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGINT, &act, NULL) < 0) {
		v2r_log_error("register signal handler error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		v2r_log_error("create listen socket error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	/* set address reuse */
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))
		== -1) {
		v2r_log_error("set socket reuse address error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(listen_port);
	if (inet_pton(AF_INET, listen_ip, &listen_addr.sin_addr) != 1) {
		v2r_log_error("convert ip '%s' error: %s", listen_ip, ERRMSG);
		exit(EXIT_FAILURE);
	}

	if (bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr))
		== -1) {
		v2r_log_error("bind socket to address error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	if (listen(listen_fd, 64) == -1) {
		v2r_log_error("listen socket error: %s", ERRMSG);
		exit(EXIT_FAILURE);
	}

	v2r_log_info("listening new connection");
	while (g_process) {
		client_fd = accept(listen_fd, NULL, NULL);
		if (client_fd == -1) {
			v2r_log_error("accept new connection error: %s", ERRMSG);
			continue;
		}
		process_connection(client_fd, &opt);
	}
	close(listen_fd);

	return 0;
}
