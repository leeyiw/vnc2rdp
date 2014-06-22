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

#include "log.h"
#include "mcs.h"
#include "tpkt.h"
#include "sec.h"

static int
v2r_mcs_parse_ber_encoding(v2r_packet_t *p, uint16_t identifier,
						   uint16_t *length)
{
	/* BER-encoding see http://en.wikipedia.org/wiki/X.690 */
	uint16_t id, len, i, l;

	if (identifier > 0xFF) {
		V2R_PACKET_READ_UINT16_BE(p, id);
	} else {
		V2R_PACKET_READ_UINT8(p, id);
	}
	if (id != identifier) {
		return -1;
	}
	V2R_PACKET_READ_UINT8(p, len);
	if (len & 0x80) {
		len &= ~0x80;
		*length = 0;
		for (i = 0; i < len; i++) {
			V2R_PACKET_READ_UINT8(p, l);
			*length = (*length << 8) | l;
		}
	} else {
		*length = len;
	}
	return 0;
}

static int
v2r_mcs_parse_client_core_data(v2r_packet_t *p, v2r_mcs_t *m)
{
	uint32_t version;

	/* version */
	V2R_PACKET_READ_UINT32_LE(p, version);
	v2r_log_info("client RDP version number: 0x%08x", version);
	/* desktopWidth */
	V2R_PACKET_SEEK_UINT16(p);
	/* desktopHeight */
	V2R_PACKET_SEEK_UINT16(p);
	/* colorDepth */
	V2R_PACKET_SEEK_UINT16(p);
	/* SASSequence */
	V2R_PACKET_SEEK_UINT16(p);
	/* keyboardLayout */
	V2R_PACKET_READ_UINT32_LE(p, m->keyboard_layout);
	v2r_log_info("client keyboard layout: 0x%08x", m->keyboard_layout);

	return 0;
}

static int
v2r_mcs_parse_client_network_data(v2r_packet_t *p, v2r_mcs_t *m)
{
	uint16_t channel_def_array_size = 0;

	V2R_PACKET_READ_UINT32_LE(p, m->channel_count);
	if (m->channel_count > MAX_CHANNELS_ALLOWED) {
		goto fail;
	}
	channel_def_array_size = m->channel_count * sizeof(channel_def_t);
	if (!V2R_PACKET_READ_REMAIN(p, channel_def_array_size)) {
		goto fail;
	}
	if (m->channel_count != 0) {
		m->channel_def_array = (channel_def_t *)malloc(channel_def_array_size);
		V2R_PACKET_READ_N(p, m->channel_def_array, channel_def_array_size);
	}

	return 0;

fail:
	return -1;
}

static int
v2r_mcs_recv_conn_init(v2r_packet_t *p, v2r_mcs_t *m)
{
	uint16_t len = 0;
	uint8_t *header = NULL;
	uint16_t type = 0, length;

	if (v2r_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	/* parse connect-initial header */
	if (v2r_mcs_parse_ber_encoding(p, BER_TAG_CONNECT_INITIAL, &len) == -1) {
		goto fail;
	}
	/* parse callingDomainSelector */
	if (v2r_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &len) == -1) {
		goto fail;
	}
	V2R_PACKET_SEEK(p, len);
	/* parse calledDomainSelector */
	if (v2r_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &len) == -1) {
		goto fail;
	}
	V2R_PACKET_SEEK(p, len);
	/* parse upwardFlag */
	if (v2r_mcs_parse_ber_encoding(p, BER_TAG_BOOLEAN, &len) == -1) {
		goto fail;
	}
	V2R_PACKET_SEEK(p, len);
	/* parse targetParameters */
	if (v2r_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &len)
		== -1) {
		goto fail;
	}
	V2R_PACKET_SEEK(p, len);
	/* parse minimumParameters */
	if (v2r_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &len)
		== -1) {
		goto fail;
	}
	V2R_PACKET_SEEK(p, len);
	/* parse maximumParameters */
	if (v2r_mcs_parse_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, &len)
		== -1) {
		goto fail;
	}
	V2R_PACKET_SEEK(p, len);
	/* parse userData */
	if (v2r_mcs_parse_ber_encoding(p, BER_TAG_OCTET_STRING, &len) == -1) {
		goto fail;
	}
	/* parse data segments */
	if (!V2R_PACKET_READ_REMAIN(p, GCCCCRQ_HEADER_LEN)) {
		goto fail;
	}
	V2R_PACKET_SEEK(p, GCCCCRQ_HEADER_LEN);
	while (V2R_PACKET_READ_REMAIN(p, TS_UD_HEADER_LEN)) {
		header = p->current;
		/* parse user data header */
		V2R_PACKET_READ_UINT16_LE(p, type);
		V2R_PACKET_READ_UINT16_LE(p, length);
		if (length < TS_UD_HEADER_LEN ||
			!V2R_PACKET_READ_REMAIN(p, length - TS_UD_HEADER_LEN)) {
			goto fail;
		}
		switch (type) {
		case CS_CORE:
			if (v2r_mcs_parse_client_core_data(p, m) == -1) {
				goto fail;
			}
			break;
		case CS_SECURITY:
			break;
		case CS_NET:
			if (v2r_mcs_parse_client_network_data(p, m) == -1) {
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
v2r_mcs_write_ber_encoding(v2r_packet_t *p, uint16_t identifier,
						   uint16_t length)
{
	/* BER-encoding see http://en.wikipedia.org/wiki/X.690 */
	if (identifier > 0xFF) {
		V2R_PACKET_WRITE_UINT16_BE(p, identifier);
	} else {
		V2R_PACKET_WRITE_UINT8(p, identifier);
	}
	if (length >= 0x80) {
		V2R_PACKET_WRITE_UINT8(p, 0x82);
		V2R_PACKET_WRITE_UINT16_BE(p, length);
	} else {
		V2R_PACKET_WRITE_UINT8(p, length);
	}
	return 0;
}

static int
v2r_mcs_send_conn_resp(v2r_packet_t *p, v2r_mcs_t *m)
{
	v2r_packet_t *u = NULL;
	uint8_t gccccrsp_header[] = {
		0x00, 0x05, 0x00, 0x14, 0x7c, 0x00, 0x01, 0x2a, 0x14, 0x76,
		0x0a, 0x01, 0x01, 0x00, 0x01, 0xc0, 0x00, 0x4d, 0x63, 0x44,
		0x6e
	};
	uint16_t i = 0, mcs_rsp_len = 0;
	uint16_t channel_count_even = m->channel_count + (m->channel_count & 1);

	u = v2r_packet_init(4096);
	if (u == NULL) {
		goto fail;
	}

	/* GCC layer header */
	V2R_PACKET_WRITE_N(u, gccccrsp_header, sizeof(gccccrsp_header));
	/* next content length */
	V2R_PACKET_WRITE_UINT8(u, 0x2C + channel_count_even * 2);

	/* Server Core Data */
	V2R_PACKET_WRITE_UINT16_LE(u, SC_CORE);
	V2R_PACKET_WRITE_UINT16_LE(u, 16);
	V2R_PACKET_WRITE_UINT32_LE(u, 0x00080004);
	V2R_PACKET_WRITE_UINT32_LE(u, m->x224->requested_protocols);
	V2R_PACKET_WRITE_UINT32_LE(u, 0x00000000);

	/* Server Security Data */
	V2R_PACKET_WRITE_UINT16_LE(u, SC_SECURITY);
	V2R_PACKET_WRITE_UINT16_LE(u, 20);
	V2R_PACKET_WRITE_UINT32_LE(u, ENCRYPTION_METHOD_NONE);
	V2R_PACKET_WRITE_UINT32_LE(u, ENCRYPTION_LEVEL_NONE);
	V2R_PACKET_WRITE_UINT32_LE(u, 0);
	V2R_PACKET_WRITE_UINT32_LE(u, 0);

	/* Server Network Data */
	V2R_PACKET_WRITE_UINT16_LE(u, SC_NET);
	V2R_PACKET_WRITE_UINT16_LE(u, 8 + 2 * channel_count_even);
	V2R_PACKET_WRITE_UINT16_LE(u, MCS_IO_CHANNEL_ID);
	V2R_PACKET_WRITE_UINT16_LE(u, m->channel_count);
	for (i = 0; i < channel_count_even; i++) {
		if (i < m->channel_count) {
			V2R_PACKET_WRITE_UINT16_LE(u, MCS_IO_CHANNEL_ID + i + 1);
		} else {
			V2R_PACKET_WRITE_UINT16_LE(u, 0);
		}
	}

	/* finish construct user data */
	V2R_PACKET_END(u);

	/* start construct entire packet */
	v2r_x224_init_packet(p);
	mcs_rsp_len = V2R_PACKET_LEN(u) + (V2R_PACKET_LEN(u) > 0x80 ? 38 : 36);
	v2r_mcs_write_ber_encoding(p, BER_TAG_CONNECT_RESPONSE, mcs_rsp_len);
	v2r_mcs_write_ber_encoding(p, BER_TAG_ENUMERATED, 1);
	V2R_PACKET_WRITE_UINT8(p, 0);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	V2R_PACKET_WRITE_UINT8(p, 0);
	v2r_mcs_write_ber_encoding(p, BER_TAG_DOMAIN_PARAMETERS, 26);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	V2R_PACKET_WRITE_UINT8(p, 34);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	V2R_PACKET_WRITE_UINT8(p, 3);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	V2R_PACKET_WRITE_UINT8(p, 0);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	V2R_PACKET_WRITE_UINT8(p, 1);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	V2R_PACKET_WRITE_UINT8(p, 0);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	V2R_PACKET_WRITE_UINT8(p, 1);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 3);
	V2R_PACKET_WRITE_UINT8(p, 0x00);
	V2R_PACKET_WRITE_UINT8(p, 0xFF);
	V2R_PACKET_WRITE_UINT8(p, 0xF8);
	v2r_mcs_write_ber_encoding(p, BER_TAG_INTEGER, 1);
	V2R_PACKET_WRITE_UINT8(p, 2);
	v2r_mcs_write_ber_encoding(p, BER_TAG_OCTET_STRING, V2R_PACKET_LEN(u));
	V2R_PACKET_WRITE_N(p, u->data, V2R_PACKET_LEN(u));

	V2R_PACKET_END(p);
	v2r_x224_send(m->x224, p);

	v2r_packet_destory(u);
	return 0;

fail:
	v2r_packet_destory(u);
	return -1;
}

static int
v2r_mcs_recv_erect_domain_req(v2r_packet_t *p, v2r_mcs_t *m)
{
	uint8_t choice;

	if (v2r_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	if (!V2R_PACKET_READ_REMAIN(p, sizeof(uint8_t))) {
		goto fail;
	}
	V2R_PACKET_READ_UINT8(p, choice);
	if ((choice >> 2) != MCS_ERECT_DOMAIN_REQUEST) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_mcs_recv_attach_user_request(v2r_packet_t *p, v2r_mcs_t *m)
{
	uint8_t choice;

	if (v2r_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	if (!V2R_PACKET_READ_REMAIN(p, sizeof(uint8_t))) {
		goto fail;
	}
	V2R_PACKET_READ_UINT8(p, choice);
	if ((choice >> 2) != MCS_ATTACH_USER_REQUEST) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_mcs_send_attach_user_confirm(v2r_packet_t *p, v2r_mcs_t *m)
{
	v2r_x224_init_packet(p);

	V2R_PACKET_WRITE_UINT8(p, (MCS_ATTACH_USER_CONFIRM << 2) | 2);
	V2R_PACKET_WRITE_UINT8(p, 0);
	/* User Channel ID */
	m->user_channel_id = MCS_IO_CHANNEL_ID + m->channel_count + 1;
	V2R_PACKET_WRITE_UINT16_BE(p, m->user_channel_id - MCS_BASE_CHANNEL_ID);

	V2R_PACKET_END(p);
	return v2r_x224_send(m->x224, p);
}

static int
v2r_mcs_join_channel(v2r_packet_t *p, v2r_mcs_t *m, uint16_t id)
{
	uint8_t choice;
	uint16_t user_id, channel_id;

	/* recieve channel join request */
	if (v2r_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	if (!V2R_PACKET_READ_REMAIN(p, 5)) {
		goto fail;
	}
	V2R_PACKET_READ_UINT8(p, choice);
	if ((choice >> 2) != MCS_CHANNEL_JOIN_REQUEST) {
		goto fail;
	}
	V2R_PACKET_READ_UINT16_BE(p, user_id);
	if (MCS_BASE_CHANNEL_ID + user_id != m->user_channel_id) {
		goto fail;
	}
	V2R_PACKET_READ_UINT16_BE(p, channel_id);
	if (channel_id != id) {
		goto fail;
	}

	/* send channel join confirm */
	v2r_x224_init_packet(p);
	V2R_PACKET_WRITE_UINT8(p, (MCS_CHANNEL_JOIN_CONFIRM << 2) | 2);
	V2R_PACKET_WRITE_UINT8(p, 0);
	V2R_PACKET_WRITE_UINT16_BE(p, m->user_channel_id - MCS_BASE_CHANNEL_ID);
	V2R_PACKET_WRITE_UINT16_BE(p, channel_id);
	V2R_PACKET_WRITE_UINT16_BE(p, channel_id);

	V2R_PACKET_END(p);
	return v2r_x224_send(m->x224, p);

fail:
	return -1;
}

static int
v2r_mcs_build_conn(v2r_mcs_t *m)
{
	uint16_t i = 0;
	v2r_packet_t *p = NULL;

	p = v2r_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	if (v2r_mcs_recv_conn_init(p, m) == -1) {
		goto fail;
	}
	if (v2r_mcs_send_conn_resp(p, m) == -1) {
		goto fail;
	}
	if (v2r_mcs_recv_erect_domain_req(p, m) == -1) {
		goto fail;
	}
	if (v2r_mcs_recv_attach_user_request(p, m) == -1) {
		goto fail;
	}
	if (v2r_mcs_send_attach_user_confirm(p, m) == -1) {
		goto fail;
	}
	/* User Channel */
	if (v2r_mcs_join_channel(p, m, m->user_channel_id) == -1) {
		goto fail;
	}
	/* MCS I/O Channel */
	if (v2r_mcs_join_channel(p, m, MCS_IO_CHANNEL_ID) == -1) {
		goto fail;
	}
	/* Static Virtual Channel */
	for (i = 0; i < m->channel_count; i++) {
		if (v2r_mcs_join_channel(p, m, MCS_IO_CHANNEL_ID + i + 1)
			== -1) {
			goto fail;
		}
	}

	v2r_packet_destory(p);
	return 0;

fail:
	v2r_packet_destory(p);
	return -1;
}

v2r_mcs_t *
v2r_mcs_init(int client_fd)
{
	v2r_mcs_t *m = NULL;

	m = (v2r_mcs_t *)malloc(sizeof(v2r_mcs_t));
	if (m == NULL) {
		goto fail;
	}
	memset(m, 0, sizeof(v2r_mcs_t));

	m->x224 = v2r_x224_init(client_fd);
	if (m->x224 == NULL) {
		goto fail;
	}

	if (v2r_mcs_build_conn(m) == -1) {
		goto fail;
	}

	return m;

fail:
	v2r_mcs_destory(m);
	return NULL;
}

void
v2r_mcs_destory(v2r_mcs_t *m)
{
	if (m == NULL) {
		return;
	}
	if (m->x224 != NULL) {
		v2r_x224_destory(m->x224);
	}
	if (m->channel_def_array != NULL) {
		free(m->channel_def_array);
	}
	free(m);
}

int
v2r_mcs_recv(v2r_mcs_t *m , v2r_packet_t *p, uint8_t *choice,
			 uint16_t *channel_id)
{
	uint16_t user_id;

	if (v2r_x224_recv(m->x224, p) == -1) {
		goto fail;
	}
	if (!V2R_PACKET_READ_REMAIN(p, 1)) {
		goto fail;
	}
	V2R_PACKET_READ_UINT8(p, *choice);
	*choice = *choice >> 2;
	switch (*choice) {
	case MCS_SEND_DATA_REQUEST:
	case MCS_SEND_DATA_INDICATION:
		V2R_PACKET_READ_UINT16_BE(p, user_id);
		if (MCS_BASE_CHANNEL_ID + user_id != m->user_channel_id) {
			goto fail;
		}
		V2R_PACKET_READ_UINT16_BE(p, *channel_id);
		V2R_PACKET_SEEK(p, 1);
		V2R_PACKET_SEEK(p, (p->current[0] & 0x80) ? 2 : 1);
		break;
	}

	return 0;

fail:
	return -1;
}

int
v2r_mcs_send(v2r_mcs_t *m, v2r_packet_t *p, uint8_t choice,
			 uint16_t channel_id)
{
	p->current = p->mcs;
	V2R_PACKET_WRITE_UINT8(p, choice << 2);
	V2R_PACKET_WRITE_UINT16_BE(p, m->user_channel_id - MCS_BASE_CHANNEL_ID);
	V2R_PACKET_WRITE_UINT16_BE(p, channel_id);
	V2R_PACKET_WRITE_UINT8(p, 0x70);
	V2R_PACKET_WRITE_UINT16_BE(p, 0x8000 | (p->end - p->mcs - 8));
	return v2r_x224_send(m->x224, p);
}

void
v2r_mcs_init_packet(v2r_packet_t *p)
{
	v2r_x224_init_packet(p);
	p->mcs = p->current;
	V2R_PACKET_SEEK(p, 8);
}
