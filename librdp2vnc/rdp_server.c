#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rdp_server.h"

r2v_rdp_server_t *
r2v_rdp_server_init(const char *ip, uint16_t port)
{
	r2v_rdp_server_t *s = NULL;
	struct sockaddr_in s_addr;

	s = (r2v_rdp_server_t *)malloc(sizeof(r2v_rdp_server_t));
	if (s == NULL) {
		goto fail;
	}
	memset(s, 0, sizeof(r2v_rdp_server_t));

	if ((s->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		goto fail;
	}

	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(port);
	if (1 != inet_pton(AF_INET, ip, &s_addr.sin_addr)) {
		goto fail;
	}

	if (-1 == bind(s->fd, (struct sockaddr *)&s_addr, sizeof(s_addr))) {
		goto fail;
	}

	if (-1 == listen(s->fd, 64)) {
		goto fail;
	}

	return s;

fail:
	if (s->fd != 0) {
		close(s->fd);
	}
	if (s != NULL) {
		free(s);
	}
	return NULL;
}

r2v_rdp_conn_t *
r2v_rdp_server_accept(r2v_rdp_server_t *s)
{
}
