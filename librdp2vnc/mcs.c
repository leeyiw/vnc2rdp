#include <stdlib.h>
#include <string.h>

#include "mcs.h"

static int
r2v_mcs_parse_ber_encoding(packet_t *p, uint16_t identifier, uint16_t *length)
{
	/* BER-encoding see http://en.wikipedia.org/wiki/X.690 */
	uint16_t id, len, i, l;

	if (identifier > 0xFF) {
		R2V_PACKET_READ_UINT16_BE(p, id);
	} else {
		R2V_PACKET_READ_UINT8(p, id);
	}
	if (id != identifier) {
		return -1;
	}
	R2V_PACKET_READ_UINT8(p, len);
	if (len & 0x80) {
		len &= ~0x80;
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
r2v_mcs_parse_client_network_data(packet_t *p, r2v_mcs_t *m)
{
	uint16_t channel_def_array_size = 0;

	R2V_PACKET_READ_UINT32_LE(p, m->channel_count);
	if (m->channel_count > MAX_CHANNELS_ALLOWED) {
		goto fail;
	}
	channel_def_array_size = m->channel_count * sizeof(channel_def_t);
	if (!R2V_PACKET_READ_REMAIN(p, channel_def_array_size)) {
		goto fail;
	}
	if (m->channel_count != 0) {
		m->channel_def_array = (channel_def_t *)malloc(channel_def_array_size);
		R2V_PACKET_READ_N(p, m->channel_def_array, channel_def_array_size);
	}

	return 0;

fail:
	return -1;
}

static int
r2v_mcs_recv_conn_init_pkt(int client_fd, packet_t *p, r2v_mcs_t *m)
{
	uint16_t len = 0;
	uint8_t *header = NULL;
	uint16_t type = 0, length;

	if (r2v_x224_recv_data_pkt(client_fd, p) == -1) {
		goto fail;
	}
	/* parse connect-initial header */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_CONNECT_INITIAL, &len) == -1) {
		goto fail;
	}
	/* parse callingDomainSelector */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &len) == -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, len);
	/* parse calledDomainSelector */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &len) == -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, len);
	/* parse upwardFlag */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_BOOLEAN, &len) == -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, len);
	/* parse targetParameters */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &len)
		== -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, len);
	/* parse minimumParameters */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &len)
		== -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, len);
	/* parse maximumParameters */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &len)
		== -1) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, len);
	/* parse userData */
	if (r2v_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &len) == -1) {
		goto fail;
	}
	/* parse data segments */
	if (!R2V_PACKET_READ_REMAIN(p, GCCCCRQ_HEADER_LEN)) {
		goto fail;
	}
	R2V_PACKET_SEEK(p, GCCCCRQ_HEADER_LEN);
	while (R2V_PACKET_READ_REMAIN(p, TS_UD_HEADER_LEN)) {
		header = p->current;
		/* parse user data header */
		R2V_PACKET_READ_UINT16_LE(p, type);
		R2V_PACKET_READ_UINT16_LE(p, length);
		if (length < TS_UD_HEADER_LEN ||
			!R2V_PACKET_READ_REMAIN(p, length - TS_UD_HEADER_LEN)) {
			goto fail;
		}
		switch (type) {
		case CS_CORE:
			break;
		case CS_SECURITY:
			break;
		case CS_NET:
			if (r2v_mcs_parse_client_network_data(p, m) == -1) {
				goto fail;
			}
			break;
		case CS_CLUSTER:
			break;
		default:
			break;
		}
		p->current = header + length;
	}

	return 0;

fail:
	return -1;
}

static int
r2v_mcs_send_conn_resp_pkt(int client_fd, packet_t *p, r2v_mcs_t *m)
{
	packet_t *u = NULL;
	uint8_t gccccrsp_header[] = {
		0x00, 0x05, 0x00, 0x14, 0x7c, 0x00, 0x01, 0x2a, 0x14, 0x76,
		0x0a, 0x01, 0x01, 0x00, 0x01, 0xc0, 0x00, 0x4d, 0x63, 0x44,
		0x6e
	};
	uint16_t channel_count_even = m->channel_count + (m->channel_count & 1);

	u = r2v_packet_init(4096);
	if (u == NULL) {
		goto fail;
	}
	/* pack userData first */
	R2V_PACKET_WRITE_N(u, gccccrsp_header, sizeof(gccccrsp_header));
	R2V_PACKET_WRITE_UINT16_BE(u, 0x80FC + channel_count_even);
	/* Server Core Data */
	R2V_PACKET_WRITE_UINT16_LE(u, SC_CORE);
	R2V_PACKET_WRITE_UINT16_LE(u, 16);
	R2V_PACKET_WRITE_UINT32_LE(u, 0x00080004);
	R2V_PACKET_WRITE_UINT32_LE(u, m->x224->requested_protocols);
	R2V_PACKET_WRITE_UINT32_LE(u, 0x00000000);

	return 0;

fail:
	r2v_packet_destory(u);
	return -1;
}

static int
r2v_mcs_build_conn(int client_fd, r2v_mcs_t *m)
{
	packet_t *p = NULL;

	p = r2v_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	if (r2v_mcs_recv_conn_init_pkt(client_fd, p, m) == -1) {
		goto fail;
	}
	if (r2v_mcs_send_conn_resp_pkt(client_fd, p, m) == -1) {
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

	if (-1 == r2v_mcs_build_conn(client_fd, m)) {
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
	if (m->channel_def_array != NULL) {
		free(m->channel_def_array);
	}
	free(m);
}
