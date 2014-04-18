#include <stdlib.h>
#include <string.h>

#include "tpkt.h"
#include "x224.h"

static int
r2v_x224_build_conn(int client_fd)
{
	packet_t *p = NULL;
	uint8_t li = 0, tpdu_code = 0;
	uint8_t type = 0, flags = 0;
	uint16_t length = 0, requested_protocols = 0;

	p = r2v_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	/* receive Client X.224 Connection Request PDU */
	r2v_tpkt_recv_pkt(client_fd, p);
	/* parse X.224 Class 0 Connection Request header */
	R2V_PACKET_READ_UINT8(p, li);
	R2V_PACKET_READ_UINT8(p, tpdu_code);
	if (tpdu_code != TPDU_CODE_CR) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, 5);
	/* parse RDP Negotiation Request */
	R2V_PACKET_READ_UINT8(p, type);
	if (type != TYPE_RDP_NEG_REQ) {
		goto fail;
	}
	R2V_PACKET_READ_UINT8(p, flags);
	R2V_PACKET_READ_UINT16_LE(p, length);
	if (length != 0x0008) {
		goto fail;
	}
	R2V_PACKET_READ_UINT32_LE(p, requested_protocols);

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
	r2v_tpkt_send_pkt(client_fd, p);

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

	if (-1 == r2v_x224_build_conn(client_fd)) {
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
r2v_x224_recv_data_pkt(int client_fd, packet_t *p)
{
	uint8_t li = 0, tpdu_code = 0;

	r2v_tpkt_recv_pkt(client_fd, p);
	/* parse X.224 data pdu header */
	R2V_PACKET_READ_UINT8(p, li);
	R2V_PACKET_READ_UINT8(p, tpdu_code);
	if (tpdu_code != TPDU_CODE_DT) {
		goto fail;
	}
	R2V_PACKET_SEEK_UINT8(p);

fail:
	return -1;
}
