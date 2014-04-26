#include <stdlib.h>
#include <string.h>

#include "mcs.h"
#include "tpkt.h"
#include "sec.h"

static int
r2v_mcs_parse_ber_encoding(r2v_packet_t *p, uint16_t identifier, uint16_t *length)
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
r2v_mcs_parse_client_network_data(r2v_packet_t *p, r2v_mcs_t *m)
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
r2v_mcs_recv_conn_init(r2v_packet_t *p, r2v_mcs_t *m)
{
	uint16_t len = 0;
	uint8_t *header = NULL;
	uint16_t type = 0, length;

	if (r2v_x224_recv(m->x224, p) == -1) {
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
r2v_mcs_write_ber_encoding(r2v_packet_t *p, uint16_t identifier, uint16_t length)
{
	/* BER-encoding see http://en.wikipedia.org/wiki/X.690 */
	if (identifier > 0xFF) {
		R2V_PACKET_WRITE_UINT16_BE(p, identifier);
	} else {
		R2V_PACKET_WRITE_UINT8(p, identifier);
	}
	if (length >= 0x80) {
		R2V_PACKET_WRITE_UINT8(p, 0x82);
		R2V_PACKET_WRITE_UINT16_BE(p, length);
	} else {
		R2V_PACKET_WRITE_UINT8(p, length);
	}
	return 0;
}

static int
r2v_mcs_send_conn_resp(r2v_packet_t *p, r2v_mcs_t *m)
{
	r2v_packet_t *u = NULL;
	uint8_t gccccrsp_header[] = {
		0x00, 0x05, 0x00, 0x14, 0x7c, 0x00, 0x01, 0x2a, 0x14, 0x76,
		0x0a, 0x01, 0x01, 0x00, 0x01, 0xc0, 0x00, 0x4d, 0x63, 0x44,
		0x6e
	};
	uint16_t i = 0, mcs_rsp_len = 0;
	uint16_t channel_count_even = m->channel_count + (m->channel_count & 1);

	u = r2v_packet_init(4096);
	if (u == NULL) {
		goto fail;
	}

	/* GCC layer header */
	R2V_PACKET_WRITE_N(u, gccccrsp_header, sizeof(gccccrsp_header));
	/* next content length */
	R2V_PACKET_WRITE_UINT8(u, 0x2C + channel_count_even * 2);

	/* Server Core Data */
	R2V_PACKET_WRITE_UINT16_LE(u, SC_CORE);
	R2V_PACKET_WRITE_UINT16_LE(u, 16);
	R2V_PACKET_WRITE_UINT32_LE(u, 0x00080004);
	R2V_PACKET_WRITE_UINT32_LE(u, m->x224->requested_protocols);
	R2V_PACKET_WRITE_UINT32_LE(u, 0x00000000);

	/* Server Security Data */
	R2V_PACKET_WRITE_UINT16_LE(u, SC_SECURITY);
	R2V_PACKET_WRITE_UINT16_LE(u, 20);
	R2V_PACKET_WRITE_UINT32_LE(u, ENCRYPTION_METHOD_NONE);
	R2V_PACKET_WRITE_UINT32_LE(u, ENCRYPTION_LEVEL_NONE);
	R2V_PACKET_WRITE_UINT32_LE(u, 0);
	R2V_PACKET_WRITE_UINT32_LE(u, 0);

	/* Server Network Data */
	R2V_PACKET_WRITE_UINT16_LE(u, SC_NET);
	R2V_PACKET_WRITE_UINT16_LE(u, 8 + 2 * channel_count_even);
	R2V_PACKET_WRITE_UINT16_LE(u, MCS_IO_CHANNEL_ID);
	R2V_PACKET_WRITE_UINT16_LE(u, m->channel_count);
	for (i = 0; i < channel_count_even; i++) {
		if (i < m->channel_count) {
			R2V_PACKET_WRITE_UINT16_LE(u, MCS_IO_CHANNEL_ID + i + 1);
		} else {
			R2V_PACKET_WRITE_UINT16_LE(u, 0);
		}
	}

	/* finish construct user data */
	R2V_PACKET_END(u);

	/* start construct entire packet */
	r2v_x224_init_packet(p);
	mcs_rsp_len = R2V_PACKET_LEN(u) + (R2V_PACKET_LEN(u) > 0x80 ? 38 : 36);
	r2v_mcs_write_ber_encoding(p, BER_TAG_CONNECT_RESPONSE, mcs_rsp_len);
	r2v_mcs_write_ber_encoding(p, BER_TAG_ENUMERATED, 1);
	R2V_PACKET_WRITE_UINT8(p, 0);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	R2V_PACKET_WRITE_UINT8(p, 0);
	r2v_mcs_write_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, 26);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	R2V_PACKET_WRITE_UINT8(p, 34);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	R2V_PACKET_WRITE_UINT8(p, 3);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	R2V_PACKET_WRITE_UINT8(p, 0);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	R2V_PACKET_WRITE_UINT8(p, 1);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	R2V_PACKET_WRITE_UINT8(p, 0);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	R2V_PACKET_WRITE_UINT8(p, 1);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 3);
	R2V_PACKET_WRITE_UINT8(p, 0x00);
	R2V_PACKET_WRITE_UINT8(p, 0xFF);
	R2V_PACKET_WRITE_UINT8(p, 0xF8);
	r2v_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	R2V_PACKET_WRITE_UINT8(p, 2);
	r2v_mcs_write_ber_encoding(p, BER_TAG_OCTET_STRING, R2V_PACKET_LEN(u));
	R2V_PACKET_WRITE_N(p, u->data, R2V_PACKET_LEN(u));

	R2V_PACKET_END(p);
	r2v_x224_send(m->x224, p);

	r2v_packet_destory(u);
	return 0;

fail:
	r2v_packet_destory(u);
	return -1;
}

static int
r2v_mcs_recv_erect_domain_req(r2v_packet_t *p, r2v_mcs_t *m)
{
	uint8_t choice;

	if (r2v_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	if (!R2V_PACKET_READ_REMAIN(p, sizeof(uint8_t))) {
		goto fail;
	}
	R2V_PACKET_READ_UINT8(p, choice);
	if ((choice >> 2) != MCS_ERECT_DOMAIN_REQUEST) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
r2v_mcs_recv_attach_user_request(r2v_packet_t *p, r2v_mcs_t *m)
{
	uint8_t choice;

	if (r2v_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	if (!R2V_PACKET_READ_REMAIN(p, sizeof(uint8_t))) {
		goto fail;
	}
	R2V_PACKET_READ_UINT8(p, choice);
	if ((choice >> 2) != MCS_ATTACH_USER_REQUEST) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
r2v_mcs_send_attach_user_confirm(r2v_packet_t *p, r2v_mcs_t *m)
{
	r2v_x224_init_packet(p);

	R2V_PACKET_WRITE_UINT8(p, (MCS_ATTACH_USER_CONFIRM << 2) | 2);
	R2V_PACKET_WRITE_UINT8(p, 0);
	/* User Channel ID */
	m->user_channel_id = MCS_IO_CHANNEL_ID + m->channel_count + 1;
	R2V_PACKET_WRITE_UINT16_BE(p, m->user_channel_id - MCS_BASE_CHANNEL_ID);

	R2V_PACKET_END(p);
	return r2v_x224_send(m->x224, p);
}

static int
r2v_mcs_join_channel(r2v_packet_t *p, r2v_mcs_t *m, uint16_t id)
{
	uint8_t choice;
	uint16_t user_id, channel_id;

	/* recieve channel join request */
	if (r2v_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	if (!R2V_PACKET_READ_REMAIN(p, 5)) {
		goto fail;
	}
	R2V_PACKET_READ_UINT8(p, choice);
	if ((choice >> 2) != MCS_CHANNEL_JOIN_REQUEST) {
		goto fail;
	}
	R2V_PACKET_READ_UINT16_BE(p, user_id);
	if (MCS_BASE_CHANNEL_ID + user_id != m->user_channel_id) {
		goto fail;
	}
	R2V_PACKET_READ_UINT16_BE(p, channel_id);
	if (channel_id != id) {
		goto fail;
	}

	/* send channel join confirm */
	r2v_x224_init_packet(p);
	R2V_PACKET_WRITE_UINT8(p, (MCS_CHANNEL_JOIN_CONFIRM << 2) | 2);
	R2V_PACKET_WRITE_UINT8(p, 0);
	R2V_PACKET_WRITE_UINT16_BE(p, m->user_channel_id - MCS_BASE_CHANNEL_ID);
	R2V_PACKET_WRITE_UINT16_BE(p, channel_id);
	R2V_PACKET_WRITE_UINT16_BE(p, channel_id);

	R2V_PACKET_END(p);
	return r2v_x224_send(m->x224, p);

fail:
	return -1;
}

static int
r2v_mcs_build_conn(r2v_mcs_t *m)
{
	uint16_t i = 0;
	r2v_packet_t *p = NULL;

	p = r2v_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	if (r2v_mcs_recv_conn_init(p, m) == -1) {
		goto fail;
	}
	if (r2v_mcs_send_conn_resp(p, m) == -1) {
		goto fail;
	}
	if (r2v_mcs_recv_erect_domain_req(p, m) == -1) {
		goto fail;
	}
	if (r2v_mcs_recv_attach_user_request(p, m) == -1) {
		goto fail;
	}
	if (r2v_mcs_send_attach_user_confirm(p, m) == -1) {
		goto fail;
	}
	/* User Channel */
	if (r2v_mcs_join_channel(p, m, m->user_channel_id) == -1) {
		goto fail;
	}
	/* MCS I/O Channel */
	if (r2v_mcs_join_channel(p, m, MCS_IO_CHANNEL_ID) == -1) {
		goto fail;
	}
	/* Static Virtual Channel */
	for (i = 0; i < m->channel_count; i++) {
		if (r2v_mcs_join_channel(p, m, MCS_IO_CHANNEL_ID + i + 1)
			== -1) {
			goto fail;
		}
	}

	r2v_packet_destory(p);
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

	if (r2v_mcs_build_conn(m) == -1) {
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

int
r2v_mcs_recv(r2v_mcs_t *m , r2v_packet_t *p, uint8_t *choice,
			 uint16_t *channel_id)
{
	uint16_t user_id;

	if (r2v_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	if (!R2V_PACKET_READ_REMAIN(p, 1)) {
		goto fail;
	}
	R2V_PACKET_READ_UINT8(p, *choice);
	*choice = *choice >> 2;
	switch (*choice) {
	case MCS_SEND_DATA_REQUEST:
	case MCS_SEND_DATA_INDICATION:
		R2V_PACKET_READ_UINT16_BE(p, user_id);
		if (MCS_BASE_CHANNEL_ID + user_id != m->user_channel_id) {
			goto fail;
		}
		R2V_PACKET_READ_UINT16_BE(p, *channel_id);
		R2V_PACKET_SEEK(p, 1);
		R2V_PACKET_SEEK(p, (p->current[0] & 0x80) ? 2 : 1);
		break;
	}

	return 0;

fail:
	return -1;
}

int
r2v_mcs_send(r2v_mcs_t *m, r2v_packet_t *p, uint8_t choice,
			 uint16_t channel_id)
{
	p->current = p->mcs;
	R2V_PACKET_WRITE_UINT8(p, choice << 2);
	R2V_PACKET_WRITE_UINT16_BE(p, m->user_channel_id - MCS_BASE_CHANNEL_ID);
	R2V_PACKET_WRITE_UINT16_BE(p, channel_id);
	R2V_PACKET_WRITE_UINT8(p, 0x70);
	R2V_PACKET_WRITE_UINT16_BE(p, 0x8000 | (p->end - p->mcs - 8));
	return r2v_x224_send(m->x224, p);
}

void
r2v_mcs_init_packet(r2v_packet_t *p)
{
	r2v_x224_init_packet(p);
	p->mcs = p->current;
	R2V_PACKET_SEEK(p, 8);
}
