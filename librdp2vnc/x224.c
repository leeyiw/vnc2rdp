#include <stdlib.h>
#include <string.h>

#include "x224.h"
#include "tpkt.h"

static int
r2v_x224_build_conn(r2v_x224_t *x)
{
	packet_t *p = NULL;
	uint8_t li = 0, tpdu_code = 0;
	uint8_t terminated = 0, type = 0, flags = 0;
	uint16_t length = 0;

	p = r2v_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	/* receive Client X.224 Connection Request PDU */
	if (r2v_tpkt_recv_pkt(x->tpkt, p) == -1) {
		goto fail;
	}
	/* parse X.224 Class 0 Connection Request header */
	R2V_PACKET_READ_UINT8(p, li);
	R2V_PACKET_READ_UINT8(p, tpdu_code);
	if (tpdu_code != TPDU_CODE_CR) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, 5);
	/* see if it is routingToken or cookie field */
	if (R2V_PACKET_READ_REMAIN(p, strlen(X224_ROUTING_TOKEN_PREFIX)) &&
		!strncmp((const char *)p->current, X224_ROUTING_TOKEN_PREFIX,
				 strlen(X224_ROUTING_TOKEN_PREFIX))) {
		R2V_PACKET_SEEK(p, strlen(X224_ROUTING_TOKEN_PREFIX));
		while (R2V_PACKET_READ_REMAIN(p, 2)) {
			if (p->current[0] == '\r' && p->current[1] == '\n') {
				R2V_PACKET_SEEK(p, 2);
				terminated = 1;
				break;
			}
			p->current++;
		}
	} else if (R2V_PACKET_READ_REMAIN(p, strlen(X224_COOKIE_PREFIX)) &&
			   !strncmp((const char *)p->current, X224_COOKIE_PREFIX,
					    strlen(X224_COOKIE_PREFIX))) {
		R2V_PACKET_SEEK(p, strlen(X224_COOKIE_PREFIX));
		while (R2V_PACKET_READ_REMAIN(p, 2)) {
			if (p->current[0] == '\r' && p->current[1] == '\n') {
				R2V_PACKET_SEEK(p, 2);
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
	if (R2V_PACKET_READ_REMAIN(p, 8)) {
		R2V_PACKET_READ_UINT8(p, type);
		if (type != TYPE_RDP_NEG_REQ) {
			goto fail;
		}
		R2V_PACKET_READ_UINT8(p, flags);
		R2V_PACKET_READ_UINT16_LE(p, length);
		if (length != 0x0008) {
			goto fail;
		}
		R2V_PACKET_READ_UINT32_LE(p, x->requested_protocols);
	}

	/* build Server X.224 Connection Confirm PDU */
	r2v_packet_reset(p);
	R2V_PACKET_SEEK(p, TPKT_HEADER_LEN);
	/* Length indicator field is set fixed to 14 */
	R2V_PACKET_WRITE_UINT8(p, 14);
	R2V_PACKET_WRITE_UINT8(p, TPDU_CODE_CC);
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	R2V_PACKET_WRITE_UINT8(p, 0);
	/* fill RDP Negotiation Response */
	R2V_PACKET_WRITE_UINT8(p, TYPE_RDP_NEG_RSP);
	R2V_PACKET_WRITE_UINT8(p, 0);
	R2V_PACKET_WRITE_UINT16_LE(p, 0x0008);
	R2V_PACKET_WRITE_UINT32_LE(p, PROTOCOL_RDP);
	/* send Server X.224 Connection Confirm PDU */
	r2v_tpkt_send_pkt(x->tpkt, p);

	return 0;

fail:
	r2v_packet_destory(p);
	return -1;
}

r2v_x224_t *
r2v_x224_init(int client_fd)
{
	r2v_x224_t *x = NULL;

	x = (r2v_x224_t *)malloc(sizeof(r2v_x224_t));
	if (x == NULL) {
		goto fail;
	}
	memset(x, 0, sizeof(r2v_x224_t));

	x->tpkt = r2v_tpkt_init(client_fd);
	if (x->tpkt == NULL) {
		goto fail;
	}

	if (-1 == r2v_x224_build_conn(x)) {
		goto fail;
	}

	return x;

fail:
	r2v_x224_destory(x);
	return NULL;
}

void
r2v_x224_destory(r2v_x224_t *x)
{
	if (x == NULL) {
		return;
	}
	free(x);
}

int
r2v_x224_recv_data_pkt(r2v_x224_t *x, packet_t *p)
{
	uint8_t li = 0, tpdu_code = 0;

	if (r2v_tpkt_recv_pkt(x->tpkt, p) == -1) {
		goto fail;
	}
	/* parse X.224 data pdu header */
	R2V_PACKET_READ_UINT8(p, li);
	R2V_PACKET_READ_UINT8(p, tpdu_code);
	if (tpdu_code != TPDU_CODE_DT) {
		goto fail;
	}
	R2V_PACKET_SEEK_UINT8(p);

	return 0;

fail:
	return -1;
}

int
r2v_x224_send_data_pkt(r2v_x224_t *x, packet_t *p)
{
	uint8_t *current = NULL;

	/* save current pointer */
	current = p->current;
	/* fill X.224 data pdu header */
	p->current = p->data + TPKT_HEADER_LEN;
	R2V_PACKET_WRITE_UINT8(p, 2);
	R2V_PACKET_WRITE_UINT8(p, TPDU_CODE_DT);
	R2V_PACKET_WRITE_UINT8(p, 0x80);
	/* restore current pointer */
	p->current = current;

	return r2v_tpkt_send_pkt(x->tpkt, p);
}
