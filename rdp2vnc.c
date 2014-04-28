#include <arpa/inet.h>
#include <stddef.h>
#include <stdlib.h>

#include "log.h"
#include "rdp.h"
#include "session.h"
#include "vnc.h"

void
process_connection(int client_fd)
{
	int server_fd;
	const char *ip = "127.0.0.1";
	const uint16_t port = 5901;
	struct sockaddr_in server_addr;

	r2v_session_t *session = NULL;
	
	session = r2v_session_init();

	/* accept RDP connection */
	session->rdp = r2v_rdp_init(client_fd);
	if (session->rdp == NULL) {
		r2v_log_error("accept new rdp connection error");
		goto fail;
	}
	r2v_log_info("accept new rdp connection success");

	/* connect to VNC server */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		r2v_log_error("create socket for VNC server error: %s", ERRMSG);
		goto fail;
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
		r2v_log_error("convert ip '%s' error: %s", ip, ERRMSG);
		goto fail;
	}
	if (connect(server_fd, (struct sockaddr *)&server_addr,
				sizeof(server_addr)) == -1) {
		r2v_log_error("connect to VNC server error: %s", ERRMSG);
		goto fail;
	}
	session->vnc = r2v_vnc_init(server_fd);
	if (session->vnc == NULL) {
		r2v_log_error("connect to vnc server error");
		goto fail;
	}
	r2v_log_info("connect to vnc server success");

fail:
	r2v_session_destory(session);
}

int
main(int argc, char *argv[])
{
	int listen_fd, client_fd;
	int optval = 1;
	struct sockaddr_in listen_addr;
	const char *ip = "0.0.0.0";
	const uint16_t port = 3389;

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
	listen_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &listen_addr.sin_addr) != 1) {
		r2v_log_error("convert ip '%s' error: %s", ip, ERRMSG);
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
	while (1) {
		client_fd = accept(listen_fd, NULL, NULL);
		if (client_fd == -1) {
			r2v_log_error("accept new connection error: %s", ERRMSG);
			continue;
		}
		process_connection(client_fd);
	}
	return 0;
}
