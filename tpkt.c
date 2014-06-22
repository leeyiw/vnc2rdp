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

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "tpkt.h"

v2r_tpkt_t *
v2r_tpkt_init(int client_fd)
{
	int optval;
	v2r_tpkt_t *t = NULL;

	t = (v2r_tpkt_t *)malloc(sizeof(v2r_tpkt_t));
	if (t == NULL) {
		goto fail;
	}
	memset(t, 0, sizeof(v2r_tpkt_t));

	t->fd = client_fd;
	/* disable Negle algorithm */
	optval = 1;
	if (setsockopt(t->fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval))
		== -1) {
		goto fail;
	}

	return t;

fail:
	v2r_tpkt_destory(t);
	return NULL;
}

void
v2r_tpkt_destory(v2r_tpkt_t *t)
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
v2r_tpkt_recv(v2r_tpkt_t *t, v2r_packet_t *p)
{
	int n = 0;
	uint8_t tpkt_version = 0;
	uint16_t tpkt_len = 0;

	v2r_packet_reset(p);

	n = recv(t->fd, p->current, TPKT_HEADER_LEN, MSG_WAITALL);
	if (n == -1) {
		v2r_log_error("recevie data from RDP client error: %s", ERRMSG);
		goto fail;
	}
	if (n == 0) {
		v2r_log_info("RDP client orderly shutdown");
		goto fail;
	}
	p->end += n;

	/* TPKT version must be 3 */
	V2R_PACKET_READ_UINT8(p, tpkt_version);
	if (tpkt_version != TPKT_VERSION) {
		goto fail;
	}
	V2R_PACKET_SEEK_UINT8(p);
	V2R_PACKET_READ_UINT16_BE(p, tpkt_len);

	n = recv(t->fd, p->current, tpkt_len - TPKT_HEADER_LEN,
			 MSG_WAITALL);
	if (n == -1) {
		v2r_log_error("recevie data from RDP client error: %s", ERRMSG);
		goto fail;
	}
	if (n == 0) {
		v2r_log_info("RDP client orderly shutdown");
		goto fail;
	}
	p->end += n;

	return 0;

fail:
	return -1;
}

int
v2r_tpkt_send(v2r_tpkt_t *t, v2r_packet_t *p)
{
	p->current = p->tpkt;
	V2R_PACKET_WRITE_UINT8(p, TPKT_VERSION);
	V2R_PACKET_WRITE_UINT8(p, 0);
	V2R_PACKET_WRITE_UINT16_BE(p, V2R_PACKET_LEN(p));

	if (-1 == send(t->fd, p->data, V2R_PACKET_LEN(p), 0)) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

void
v2r_tpkt_init_packet(v2r_packet_t *p)
{
	v2r_packet_reset(p);
	p->tpkt = p->current;
	V2R_PACKET_SEEK(p, TPKT_HEADER_LEN);
}
