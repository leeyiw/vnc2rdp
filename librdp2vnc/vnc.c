#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "vnc.h"

static int
r2v_vnc_recv(r2v_vnc_t *v)
{
	int n = 0;

	r2v_packet_reset(v->packet);

	n = recv(v->fd, v->packet->current, v->packet->max_len, 0);
	if (n == -1 || n == 0) {
		goto fail;
	}
	v->packet->end += n;

	return 0;

fail:
	return -1;
}

static int
r2v_vnc_send(r2v_vnc_t *v)
{
	int n = 0;

	n = send(v->fd, v->packet->data, R2V_PACKET_LEN(v->packet), 0);
	if (n == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
r2v_vnc_build_conn(r2v_vnc_t *v)
{
	/* receive ProtocolVersion */
	if (r2v_vnc_recv(v) == -1) {
		goto fail;
	}
	/* send ProtocolVersion */
	r2v_packet_reset(v->packet);
	R2V_PACKET_WRITE_N(v->packet, RFB_PROTOCOL_VERSION,
					   strlen(RFB_PROTOCOL_VERSION));
	R2V_PACKET_END(v->packet);
	if (r2v_vnc_send(v) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

r2v_vnc_t *
r2v_vnc_init(int server_fd)
{
	r2v_vnc_t *v = NULL;

	v = (r2v_vnc_t *)malloc(sizeof(r2v_vnc_t));
	if (v == NULL) {
		goto fail;
	}
	memset(v, 0, sizeof(r2v_vnc_t));

	v->fd = server_fd;

	v->packet = r2v_packet_init(65535);
	if (v->packet == NULL) {
		goto fail;
	}

	if (r2v_vnc_build_conn(v) == -1) {
		goto fail;
	}

	return v;

fail:
	r2v_vnc_destory(v);
	return NULL;
}

void
r2v_vnc_destory(r2v_vnc_t *v)
{
	if (v == NULL) {
		return;
	}
	if (v->fd != 0) {
		close(v->fd);
	}
	if (v->packet != NULL) {
		r2v_packet_destory(v->packet);
	}
	free(v);
}
