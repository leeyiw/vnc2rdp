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

#include <stdlib.h>
#include <string.h>

#include "x224.h"
#include "tpkt.h"

static int
v2r_x224_build_conn(v2r_x224_t *x)
{
	v2r_packet_t *p = NULL;
	uint8_t li = 0, tpdu_code = 0;
	uint8_t terminated = 0, type = 0, flags = 0;
	uint16_t length = 0;

	p = v2r_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	/* receive Client X.224 Connection Request PDU */
	if (v2r_tpkt_recv(x->tpkt, p) == -1) {
		goto fail;
	}
	/* parse X.224 Class 0 Connection Request header */
	V2R_PACKET_READ_UINT8(p, li);
	V2R_PACKET_READ_UINT8(p, tpdu_code);
	if (tpdu_code != TPDU_CODE_CR) {
		goto fail;
	}
	V2R_PACKET_SEEK(p, 5);
	/* see if it is routingToken or cookie field */
	if (V2R_PACKET_READ_REMAIN(p, strlen(X224_ROUTING_TOKEN_PREFIX)) &&
		!strncmp((const char *)p->current, X224_ROUTING_TOKEN_PREFIX,
				 strlen(X224_ROUTING_TOKEN_PREFIX))) {
		V2R_PACKET_SEEK(p, strlen(X224_ROUTING_TOKEN_PREFIX));
		while (V2R_PACKET_READ_REMAIN(p, 2)) {
			if (p->current[0] == '\r' && p->current[1] == '\n') {
				V2R_PACKET_SEEK(p, 2);
				terminated = 1;
				break;
			}
			p->current++;
		}
	} else if (V2R_PACKET_READ_REMAIN(p, strlen(X224_COOKIE_PREFIX)) &&
			   !strncmp((const char *)p->current, X224_COOKIE_PREFIX,
					    strlen(X224_COOKIE_PREFIX))) {
		V2R_PACKET_SEEK(p, strlen(X224_COOKIE_PREFIX));
		while (V2R_PACKET_READ_REMAIN(p, 2)) {
			if (p->current[0] == '\r' && p->current[1] == '\n') {
				V2R_PACKET_SEEK(p, 2);
				terminated = 1;
				break;
			}
			p->current++;
		}
	} else {
		terminated = 1;
	}
	if (!terminated) {
		goto fail;
	}

	/* parse RDP Negotiation Request if exists */
	if (V2R_PACKET_READ_REMAIN(p, 8)) {
		V2R_PACKET_READ_UINT8(p, type);
		if (type != TYPE_RDP_NEG_REQ) {
			goto fail;
		}
		V2R_PACKET_READ_UINT8(p, flags);
		V2R_PACKET_READ_UINT16_LE(p, length);
		if (length != 0x0008) {
			goto fail;
		}
		V2R_PACKET_READ_UINT32_LE(p, x->requested_protocols);
	}

	/* build Server X.224 Connection Confirm PDU */
	v2r_tpkt_init_packet(p);
	/* Length indicator field is set fixed to 14 */
	V2R_PACKET_WRITE_UINT8(p, 14);
	V2R_PACKET_WRITE_UINT8(p, TPDU_CODE_CC);
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	V2R_PACKET_WRITE_UINT8(p, 0);
	/* fill RDP Negotiation Response */
	V2R_PACKET_WRITE_UINT8(p, TYPE_RDP_NEG_RSP);
	V2R_PACKET_WRITE_UINT8(p, 0);
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0008);
	V2R_PACKET_WRITE_UINT32_LE(p, PROTOCOL_RDP);
	/* send Server X.224 Connection Confirm PDU */
	V2R_PACKET_END(p);
	v2r_tpkt_send(x->tpkt, p);

	v2r_packet_destory(p);
	return 0;

fail:
	v2r_packet_destory(p);
	return -1;
}

v2r_x224_t *
v2r_x224_init(int client_fd)
{
	v2r_x224_t *x = NULL;

	x = (v2r_x224_t *)malloc(sizeof(v2r_x224_t));
	if (x == NULL) {
		goto fail;
	}
	memset(x, 0, sizeof(v2r_x224_t));

	x->tpkt = v2r_tpkt_init(client_fd);
	if (x->tpkt == NULL) {
		goto fail;
	}

	if (-1 == v2r_x224_build_conn(x)) {
		goto fail;
	}

	return x;

fail:
	v2r_x224_destory(x);
	return NULL;
}

void
v2r_x224_destory(v2r_x224_t *x)
{
	if (x == NULL) {
		return;
	}
	if (x->tpkt != NULL) {
		v2r_tpkt_destory(x->tpkt);
	}
	free(x);
}

int
v2r_x224_recv(v2r_x224_t *x, v2r_packet_t *p)
{
	uint8_t li = 0, tpdu_code = 0;

	if (v2r_tpkt_recv(x->tpkt, p) == -1) {
		goto fail;
	}
	/* parse X.224 data pdu header */
	V2R_PACKET_READ_UINT8(p, li);
	V2R_PACKET_READ_UINT8(p, tpdu_code);
	if (tpdu_code != TPDU_CODE_DT) {
		goto fail;
	}
	V2R_PACKET_SEEK_UINT8(p);

	return 0;

fail:
	return -1;
}

int
v2r_x224_send(v2r_x224_t *x, v2r_packet_t *p)
{
	p->current = p->x224;
	V2R_PACKET_WRITE_UINT8(p, 2);
	V2R_PACKET_WRITE_UINT8(p, TPDU_CODE_DT);
	V2R_PACKET_WRITE_UINT8(p, 0x80);

	return v2r_tpkt_send(x->tpkt, p);
}

void
v2r_x224_init_packet(v2r_packet_t *p)
{
	v2r_tpkt_init_packet(p);
	p->x224 = p->current;
	V2R_PACKET_SEEK(p, X224_DATA_HEADER_LEN);
}
