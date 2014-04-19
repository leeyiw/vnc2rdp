#include <stdlib.h>
#include <string.h>

#include "mcs.h"

static int
r2v_mcs_parse_ber_encoding(packet_t *p, uint16_t identifier, uint16_t *length)
{
	/* BER-encoding see http://en.wikipedia.org/wiki/X.690 */
	uint16_t id, len, i, l;

	if (identifier > 0xFF) {
		R2V_PACKET_READ_UINT16_LE(p, id);
	} else {
		R2V_PACKET_READ_UINT8(p, id);
	}
	if (id != identifier) {
		return -1;
	}
	R2V_PACKET_READ_UINT8(p, len);
	if (len & 0x80) {
		len &= ~0x80;
		// TODO finish this
		*length = 0;
		for (i = 0; i < len; i++) {
			R2V_PACKET_READ_UINT8(p, l);
			*length = (*length << 8) | l;
		}
	} else {
		*length = len;
	}
	return 0;
}

static int
r2v_mcs_recv_conn_init_pkt(int client_fd, packet_t *p)
{
	int length = 0;

	if (r2v_x224_recv_data_pkt(client_fd, p) == -1) {
		goto fail;
	}
	/* parse connect-initial header */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_CONNECT_INITIAL, &length) == -1) {
		goto fail;
	}
	/* parse callingDomainSelector */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &length) == -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, length);
	/* parse calledDomainSelector */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &length) == -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, length);
	/* parse calledDomainSelector */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &length) == -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, length);
	/* parse upwardFlag */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_BOOLEAN, &length) == -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, length);
	/* parse targetParameters */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &length)
		== -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, length);
	/* parse minimumParameters */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &length)
		== -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, length);
	/* parse maximumParameters */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &length)
		== -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, length);
	/* TODO parse GCC pdu and mcs data */
	return 0;

fail:
	return -1;
}

static int
r2v_mcs_build_conn(int client_fd)
{
	packet_t *p = NULL;

	p = r2v_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	if (r2v_mcs_recv_conn_init_pkt(client_fd, p) == -1) {
		goto fail;
	}

	return 0;

fail:
	r2v_packet_destory(p);
	return -1;
}

r2v_mcs_t *
r2v_mcs_init(int client_fd)
{
	r2v_mcs_t *m = NULL;

	m = (r2v_mcs_t *)malloc(sizeof(r2v_mcs_t));
	if (m == NULL) {
		goto fail;
	}
	memset(m, 0, sizeof(r2v_mcs_t));

	m->x224 = r2v_x224_init(client_fd);
	if (m->x224 == NULL) {
		goto fail;
	}

	if (-1 == r2v_mcs_build_conn(client_fd)) {
		goto fail;
	}

	return m;

fail:
	r2v_mcs_destory(m);
	return NULL;
}

void
r2v_mcs_destory(r2v_mcs_t *m)
{
	if (m == NULL) {
		return;
	}
	if (m->x224 != NULL) {
		r2v_x224_destory(m->x224);
	}
	free(m);
}
