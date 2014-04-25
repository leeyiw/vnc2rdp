#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "tpkt.h"

r2v_tpkt_t *
r2v_tpkt_init(int client_fd)
{
	r2v_tpkt_t *t = NULL;

	t = (r2v_tpkt_t *)malloc(sizeof(r2v_tpkt_t));
	if (t == NULL) {
		goto fail;
	}
	memset(t, 0, sizeof(r2v_tpkt_t));

	t->fd = client_fd;

	return t;

fail:
	r2v_tpkt_destory(t);
	return NULL;
}

void
r2v_tpkt_destory(r2v_tpkt_t *t)
{
	if (t == NULL) {
		return;
	}
	if (t->fd != 0) {
		close(t->fd);
	}
	free(t);
}

int
r2v_tpkt_recv_pkt(r2v_tpkt_t *t, packet_t *p)
{
	int n = 0;
	uint8_t tpkt_version = 0;

	r2v_packet_reset(p);

	n = recv(t->fd, p->current, TPKT_HEADER_LEN, MSG_WAITALL);
	if (n == -1 || n == 0) {
		goto fail;
	}

	/* TPKT version must be 3 */
	R2V_PACKET_READ_UINT8(p, tpkt_version);
	if (tpkt_version != TPKT_VERSION) {
		goto fail;
	}
	R2V_PACKET_SEEK_UINT8(p);
	R2V_PACKET_READ_UINT16_BE(p, p->total_len);

	n = recv(t->fd, p->current, p->total_len - TPKT_HEADER_LEN,
			 MSG_WAITALL);
	if (n == -1 || n == 0) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

int
r2v_tpkt_send_pkt(r2v_tpkt_t *t, packet_t *p)
{
	p->total_len = p->current - p->data;

	p->current = p->data;
	R2V_PACKET_WRITE_UINT8(p, TPKT_VERSION);
	R2V_PACKET_WRITE_UINT8(p, 0);
	R2V_PACKET_WRITE_UINT16_BE(p, p->total_len);

	if (-1 == send(t->fd, p->data, p->total_len, 0)) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}
