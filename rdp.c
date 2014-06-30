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

#include "capabilities.h"
#include "input.h"
#include "license.h"
#include "log.h"
#include "orders.h"
#include "rdp.h"
#include "vnc.h"

static int
v2r_rdp_recv_client_info(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint16_t sec_flags = 0, channel_id = 0;

	if (v2r_sec_recv(r->sec, p, &sec_flags, &channel_id) == -1) {
		goto fail;
	}
	if (sec_flags != SEC_INFO_PKT || channel_id != MCS_IO_CHANNEL_ID) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_rdp_send_license_error(v2r_rdp_t *r, v2r_packet_t *p)
{
	v2r_sec_init_packet(p);

	/* bMsgType */
	V2R_PACKET_WRITE_UINT8(p, ERROR_ALERT);
	/* flags */
	V2R_PACKET_WRITE_UINT8(p, PREAMBLE_VERSION_3_0);
	/* wMsgSize */
	V2R_PACKET_WRITE_UINT16_LE(p, 16);
	/* dwErrorCode */
	V2R_PACKET_WRITE_UINT32_LE(p, STATUS_VALID_CLIENT);
	/* dwStateTransition */
	V2R_PACKET_WRITE_UINT32_LE(p, ST_NO_TRANSITION);
	/* wBlobType */
	V2R_PACKET_WRITE_UINT16_LE(p, BB_ERROR_BLOB);
	/* wBlobLen */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);

	V2R_PACKET_END(p);
	return v2r_sec_send(r->sec, p, SEC_LICENSE_PKT, MCS_IO_CHANNEL_ID);
}

static int
v2r_rdp_send_demand_active(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint8_t *cap_size_ptr = NULL;
	uint16_t *length_combined_capabilities = NULL;
	share_data_hdr_t hdr;

	v2r_rdp_init_packet(p, sizeof(share_ctrl_hdr_t));
	/* shareControlHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DEMANDACTIVEPDU;
	/* shareId */
	V2R_PACKET_WRITE_UINT32_LE(p, 0x1000 + r->sec->mcs->user_channel_id);
	/* lengthSourceDescriptor */
	V2R_PACKET_WRITE_UINT16_LE(p, 4);
	/* lengthCombinedCapabilities: mark this place */
	length_combined_capabilities = (uint16_t *)p->current;
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* sourceDescriptor */
	V2R_PACKET_WRITE_N(p, "RDP", 4);
	/* numberCapabilities */
	cap_size_ptr = p->current;
	V2R_PACKET_WRITE_UINT16_LE(p, v2r_cap_get_write_count());
	/* pad2Octets */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* capabilitySets */
	v2r_cap_write_caps(r, p);
	*length_combined_capabilities = p->current - cap_size_ptr;
	/* sessionId */
	V2R_PACKET_WRITE_UINT32_LE(p, 0);

	V2R_PACKET_END(p);
	return v2r_rdp_send(r, p, &hdr);
}

static int
v2r_rdp_recv_confirm_active(v2r_rdp_t *r, v2r_packet_t *p)
{
	share_data_hdr_t hdr;

	if (v2r_rdp_recv(r, p, &hdr) == -1) {
		goto fail;
	}
	if (hdr.share_ctrl_hdr.type != PDUTYPE_CONFIRMACTIVEPDU) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_rdp_recv_synchronize(v2r_rdp_t *r, v2r_packet_t *p)
{
	share_data_hdr_t hdr;

	if (v2r_rdp_recv(r, p, &hdr) == -1) {
		goto fail;
	}
	if (hdr.share_ctrl_hdr.type != PDUTYPE_DATAPDU ||
		hdr.pdu_type2 != PDUTYPE2_SYNCHRONIZE) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_rdp_recv_control_cooperate(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint16_t action = 0;
	share_data_hdr_t hdr;

	if (v2r_rdp_recv(r, p, &hdr) == -1) {
		goto fail;
	}
	if (hdr.share_ctrl_hdr.type != PDUTYPE_DATAPDU ||
		hdr.pdu_type2 != PDUTYPE2_CONTROL) {
		goto fail;
	}
	/* action */
	V2R_PACKET_READ_UINT16_LE(p, action);
	if (action != CTRLACTION_COOPERATE) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_rdp_recv_control_request(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint16_t action = 0;
	share_data_hdr_t hdr;

	if (v2r_rdp_recv(r, p, &hdr) == -1) {
		goto fail;
	}
	if (hdr.share_ctrl_hdr.type != PDUTYPE_DATAPDU ||
		hdr.pdu_type2 != PDUTYPE2_CONTROL) {
		goto fail;
	}
	/* action */
	V2R_PACKET_READ_UINT16_LE(p, action);
	if (action != CTRLACTION_REQUEST_CONTROL) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_rdp_recv_font_list(v2r_rdp_t *r, v2r_packet_t *p)
{
	share_data_hdr_t hdr;

	if (v2r_rdp_recv(r, p, &hdr) == -1) {
		goto fail;
	}
	if (hdr.share_ctrl_hdr.type != PDUTYPE_DATAPDU ||
		hdr.pdu_type2 != PDUTYPE2_FONTLIST) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_rdp_send_synchronize(v2r_rdp_t *r, v2r_packet_t *p)
{
	share_data_hdr_t hdr;

	v2r_rdp_init_packet(p, sizeof(share_data_hdr_t));
	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_SYNCHRONIZE;
	/* messageType */
	V2R_PACKET_WRITE_UINT16_LE(p, SYNCMSGTYPE_SYNC);
	/* targetUser */
	V2R_PACKET_WRITE_UINT16_LE(p, MCS_IO_CHANNEL_ID);

	V2R_PACKET_END(p);
	return v2r_rdp_send(r, p, &hdr);
}

static int
v2r_rdp_send_control_cooperate(v2r_rdp_t *r, v2r_packet_t *p)
{
	share_data_hdr_t hdr;

	v2r_rdp_init_packet(p, sizeof(share_data_hdr_t));
	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_CONTROL;
	/* action */
	V2R_PACKET_WRITE_UINT16_LE(p, CTRLACTION_COOPERATE);
	/* grantId */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* controlId */
	V2R_PACKET_WRITE_UINT32_LE(p, 0);

	V2R_PACKET_END(p);
	return v2r_rdp_send(r, p, &hdr);
}

static int
v2r_rdp_send_control_grant_control(v2r_rdp_t *r, v2r_packet_t *p)
{
	share_data_hdr_t hdr;

	v2r_rdp_init_packet(p, sizeof(share_data_hdr_t));
	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_CONTROL;
	/* action */
	V2R_PACKET_WRITE_UINT16_LE(p, CTRLACTION_GRANTED_CONTROL);
	/* grantId */
	V2R_PACKET_WRITE_UINT16_LE(p, r->sec->mcs->user_channel_id);
	/* controlId */
	V2R_PACKET_WRITE_UINT32_LE(p, 0x03EA);

	V2R_PACKET_END(p);
	return v2r_rdp_send(r, p, &hdr);
}

static int
v2r_rdp_send_font_map(v2r_rdp_t *r, v2r_packet_t *p)
{
	share_data_hdr_t hdr;

	v2r_rdp_init_packet(p, sizeof(share_data_hdr_t));
	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_FONTMAP;
	/* numberEntries */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* totalNumEntries */
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	/* mapFlags */
	V2R_PACKET_WRITE_UINT16_LE(p, FONTMAP_FIRST|FONTMAP_LAST);
	/* entrySize */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0004);

	V2R_PACKET_END(p);
	return v2r_rdp_send(r, p, &hdr);
}

static int
v2r_rdp_build_conn(v2r_rdp_t *r)
{
	v2r_packet_t *p = NULL;

	p = v2r_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	if (v2r_rdp_recv_client_info(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_send_license_error(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_send_demand_active(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_recv_confirm_active(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_recv_synchronize(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_recv_control_cooperate(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_recv_control_request(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_recv_font_list(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_send_synchronize(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_send_control_cooperate(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_send_control_grant_control(r, p) == -1) {
		goto fail;
	}
	if (v2r_rdp_send_font_map(r, p) == -1) {
		goto fail;
	}

	v2r_packet_destory(p);
	return 0;

fail:
	v2r_packet_destory(p);
	return -1;
}

v2r_rdp_t *
v2r_rdp_init(int client_fd, v2r_session_t *s)
{
	v2r_rdp_t *r = NULL;

	r = (v2r_rdp_t *)malloc(sizeof(v2r_rdp_t));
	if (r == NULL) {
		goto fail;
	}
	memset(r, 0, sizeof(v2r_rdp_t));

	r->session = s;

	r->packet = v2r_packet_init(65535);
	if (r->packet == NULL) {
		goto fail;
	}

	r->sec = v2r_sec_init(client_fd);
	if (r->sec == NULL) {
		goto fail;
	}

	r->allow_display_updates = ALLOW_DISPLAY_UPDATES;
	/* find keymap by keyboard layout, if there is no fit keymap currently,
	 * vnc2rdp will continue work but don't provide keyboard function */
	r->keymap = get_keymap_by_layout(r->sec->mcs->keyboard_layout);
	if (r->keymap == NULL) {
		v2r_log_error("unsupported keyboard layout: 0x%08x",
					  r->sec->mcs->keyboard_layout);
	}

	if (v2r_rdp_build_conn(r) == -1) {
		goto fail;
	}

	return r;

fail:
	v2r_rdp_destory(r);
	return NULL;
}

void
v2r_rdp_destory(v2r_rdp_t *r)
{
	if (r == NULL) {
		return;
	}
	if (r->packet != NULL) {
		v2r_packet_destory(r->packet);
	}
	if (r->sec != NULL) {
		v2r_sec_destory(r->sec);
	}
	free(r);
}

void
v2r_rdp_init_packet(v2r_packet_t *p, uint16_t offset)
{
	v2r_mcs_init_packet(p);
	p->rdp = p->current;
	V2R_PACKET_SEEK(p, offset);
}

int
v2r_rdp_recv(v2r_rdp_t *r, v2r_packet_t *p, share_data_hdr_t *hdr)
{
	uint8_t choice;
	uint16_t channel_id;

	if (v2r_mcs_recv(r->sec->mcs, p, &choice, &channel_id) == -1) {
		goto fail;
	}
	if (!V2R_PACKET_READ_REMAIN(p, sizeof(share_ctrl_hdr_t))) {
		goto fail;
	}
	V2R_PACKET_READ_N(p, &(hdr->share_ctrl_hdr), sizeof(share_ctrl_hdr_t));
	if (hdr->share_ctrl_hdr.version_low != TS_PROTOCOL_VERSION ||
		hdr->share_ctrl_hdr.version_high != 0x00) {
		goto fail;
	}
	if (hdr->share_ctrl_hdr.type == PDUTYPE_DATAPDU) {
		V2R_PACKET_READ_N(p, (void *)hdr + sizeof(share_ctrl_hdr_t),
						  sizeof(share_data_hdr_t) - sizeof(share_ctrl_hdr_t));
	}

	return 0;

fail:
	return -1;
}

int
v2r_rdp_send(v2r_rdp_t *r, v2r_packet_t *p, share_data_hdr_t *hdr)
{
	hdr->share_ctrl_hdr.total_length = p->end - p->rdp;
	hdr->share_ctrl_hdr.version_low = TS_PROTOCOL_VERSION;
	hdr->share_ctrl_hdr.version_high = 0x00;
	hdr->share_ctrl_hdr.pdu_source = MCS_IO_CHANNEL_ID;
	if (hdr->share_ctrl_hdr.type == PDUTYPE_DATAPDU) {
		hdr->share_id = 0x1000 + r->sec->mcs->user_channel_id;
		hdr->pad1 = 0;
		hdr->stream_id = STREAM_LOW;
		hdr->uncompressed_length = p->end - p->rdp - 14;
		hdr->compressed_type = 0;
		hdr->compressed_length = 0;
	}

	p->current = p->rdp;
	if (hdr->share_ctrl_hdr.type == PDUTYPE_DATAPDU) {
		V2R_PACKET_WRITE_N(p, hdr, sizeof(share_data_hdr_t));
	} else {
		V2R_PACKET_WRITE_N(p, &(hdr->share_ctrl_hdr), sizeof(share_ctrl_hdr_t));
	}

	return v2r_mcs_send(r->sec->mcs, p, MCS_SEND_DATA_INDICATION,
						MCS_IO_CHANNEL_ID);
}

int
v2r_rdp_send_bitmap_update(v2r_rdp_t *r, uint16_t left, uint16_t top,
						   uint16_t right, uint16_t bottom,
						   uint16_t width, uint16_t height,
						   uint16_t bpp, uint16_t bitmap_length,
						   uint8_t *bitmap_data)
{
	share_data_hdr_t hdr;

	v2r_rdp_init_packet(r->packet, sizeof(share_data_hdr_t));

	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_UPDATE;
	/* updateType */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, UPDATETYPE_BITMAP);
	/* numberRectangles */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, 1);
	/* destLeft */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, left);
	/* destTop */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, top);
	/* destRight */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, right);
	/* destBottom */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, bottom);
	/* width */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, width);
	/* height */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, height);
	/* bitsPerPixel */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, bpp);
	/* flags */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, 0);
	/* bitmapLength */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, bitmap_length);
	/* bitmapDataStream */
	V2R_PACKET_WRITE_N(r->packet, bitmap_data, bitmap_length);

	/* send packet */
	V2R_PACKET_END(r->packet);
	if (v2r_rdp_send(r, r->packet, &hdr) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

int
v2r_rdp_send_palette_update(v2r_rdp_t *r, uint32_t number_colors,
							uint8_t (*palette_entries)[3])
{
	uint32_t i;
	share_data_hdr_t hdr;

	v2r_rdp_init_packet(r->packet, sizeof(share_data_hdr_t));

	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_UPDATE;
	/* updateType */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, UPDATETYPE_PALETTE);
	/* pad2Octets */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, 0);
	/* numberColors */
	V2R_PACKET_WRITE_UINT32_LE(r->packet, number_colors);
	/* paletteEntries */
	V2R_PACKET_WRITE_N(r->packet, palette_entries,
					   number_colors * sizeof(*palette_entries));

	/* send packet */
	V2R_PACKET_END(r->packet);
	if (v2r_rdp_send(r, r->packet, &hdr) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

int
v2r_rdp_send_scrblt_order(v2r_rdp_t *r, uint16_t left, uint16_t top,
						   uint16_t width, uint16_t height,
						   uint16_t x_src, uint16_t y_src)
{
	share_data_hdr_t hdr;

	v2r_rdp_init_packet(r->packet, sizeof(share_data_hdr_t));

	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_UPDATE;
	/* updateType */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, UPDATETYPE_ORDERS);
	/* pad2OctetsA */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, 0);
	/* numberOrders */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, 1);
	/* pad2OctetsB */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, 0);

	/* orderData */

	/* controlFlags */
	V2R_PACKET_WRITE_UINT8(r->packet, TS_STANDARD|TS_TYPE_CHANGE);
	/* orderType */
	V2R_PACKET_WRITE_UINT8(r->packet, TS_ENC_SCRBLT_ORDER);
	/* fieldFlags */
	V2R_PACKET_WRITE_UINT8(r->packet, 0x7F);
	/* nLeftRect */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, left);
	/* nTopRect */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, top);
	/* nWidth */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, width);
	/* nHeight */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, height);
	/* bRop */
	V2R_PACKET_WRITE_UINT8(r->packet, 0xCC);
	/* nXSrc */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, x_src);
	/* nYSrc */
	V2R_PACKET_WRITE_UINT16_LE(r->packet, y_src);

	/* send packet */
	V2R_PACKET_END(r->packet);
	if (v2r_rdp_send(r, r->packet, &hdr) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_rdp_process_suppress_output(v2r_rdp_t *r, v2r_packet_t *p)
{
	uint16_t left, top, right, bottom;
	v2r_vnc_t *v = r->session->vnc;

	V2R_PACKET_READ_UINT8(p, r->allow_display_updates);
	v2r_log_debug("client send suppress output with allow_display_updates: %d",
				  r->allow_display_updates);

	if (r->allow_display_updates == ALLOW_DISPLAY_UPDATES) {
		V2R_PACKET_READ_UINT16_LE(p, left);
		V2R_PACKET_READ_UINT16_LE(p, top);
		V2R_PACKET_READ_UINT16_LE(p, right);
		V2R_PACKET_READ_UINT16_LE(p, bottom);
		v2r_log_debug("with desktop rect: %d,%d,%d,%d",
					  left, top, right, bottom);
		if (v2r_vnc_send_fb_update_req(v, 0, 0, 0, v->framebuffer_width,
									   v->framebuffer_height) == -1) {
			goto fail;
		}
	}

	return 0;

fail:
	return -1;
}

static int
v2r_rdp_process_data(v2r_rdp_t *r, v2r_packet_t *p, const share_data_hdr_t *hdr)
{
	switch (hdr->pdu_type2) {
	case PDUTYPE2_INPUT:
		if (v2r_input_process(r, p) == -1) {
			goto fail;
		}
		break;
	case PDUTYPE2_SUPPRESS_OUTPUT:
		if (v2r_rdp_process_suppress_output(r, p) == -1) {
			goto fail;
		}
		break;
	case PDUTYPE2_SHUTDOWN_REQUEST:
		/* when client send shutdown request, we should close connection 
		 * immediately, see [MS-RDPBCGR 1.3.1.4.1] */
		v2r_log_debug("client send shutdown request");
		goto fail;
	default:
		v2r_log_warn("unknown data pdu type: 0x%x", hdr->pdu_type2);
		break;
	}

	return 0;

fail:
	return -1;
}

int
v2r_rdp_process(v2r_rdp_t *r)
{
	share_data_hdr_t hdr;

	if (v2r_rdp_recv(r, r->packet, &hdr) == -1) {
		goto fail;
	}
	switch (hdr.share_ctrl_hdr.type) {
	case PDUTYPE_DEMANDACTIVEPDU:
		break;
	case PDUTYPE_DATAPDU:
		if (v2r_rdp_process_data(r, r->packet, &hdr) == -1) {
			goto fail;
		}
		break;
	}

	return 0;

fail:
	return -1;
}
