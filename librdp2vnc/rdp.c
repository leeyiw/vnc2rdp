#include <stdlib.h>
#include <string.h>

#include "capabilities.h"
#include "input.h"
#include "license.h"
#include "log.h"
#include "rdp.h"
#include "vnc.h"

static int
r2v_rdp_recv_client_info(r2v_rdp_t *r, r2v_packet_t *p)
{
	uint16_t sec_flags = 0, channel_id = 0;

	if (r2v_sec_recv(r->sec, p, &sec_flags, &channel_id) == -1) {
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
r2v_rdp_send_license_error(r2v_rdp_t *r, r2v_packet_t *p)
{
	r2v_sec_init_packet(p);

	/* bMsgType */
	R2V_PACKET_WRITE_UINT8(p, ERROR_ALERT);
	/* flags */
	R2V_PACKET_WRITE_UINT8(p, PREAMBLE_VERSION_3_0);
	/* wMsgSize */
	R2V_PACKET_WRITE_UINT16_LE(p, 16);
	/* dwErrorCode */
	R2V_PACKET_WRITE_UINT32_LE(p, STATUS_VALID_CLIENT);
	/* dwStateTransition */
	R2V_PACKET_WRITE_UINT32_LE(p, ST_NO_TRANSITION);
	/* wBlobType */
	R2V_PACKET_WRITE_UINT16_LE(p, BB_ERROR_BLOB);
	/* wBlobLen */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);

	R2V_PACKET_END(p);
	return r2v_sec_send(r->sec, p, SEC_LICENSE_PKT, MCS_IO_CHANNEL_ID);
}

static int
r2v_rdp_send_demand_active(r2v_rdp_t *r, r2v_packet_t *p)
{
	uint8_t *cap_size_ptr = NULL;
	uint16_t *length_combined_capabilities = NULL;
	share_data_hdr_t hdr;

	r2v_rdp_init_packet(p, sizeof(share_ctrl_hdr_t));
	/* shareControlHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DEMANDACTIVEPDU;
	/* shareId */
	R2V_PACKET_WRITE_UINT32_LE(p, 0x1000 + r->sec->mcs->user_channel_id);
	/* lengthSourceDescriptor */
	R2V_PACKET_WRITE_UINT16_LE(p, 4);
	/* lengthCombinedCapabilities: mark this place */
	length_combined_capabilities = (uint16_t *)p->current;
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* sourceDescriptor */
	R2V_PACKET_WRITE_N(p, "RDP", 4);
	/* numberCapabilities */
	cap_size_ptr = p->current;
	R2V_PACKET_WRITE_UINT16_LE(p, r2v_cap_get_write_count());
	/* pad2Octets */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* capabilitySets */
	r2v_cap_write_caps(r, p);
	*length_combined_capabilities = p->current - cap_size_ptr;
	/* sessionId */
	R2V_PACKET_WRITE_UINT32_LE(p, 0);

	R2V_PACKET_END(p);
	return r2v_rdp_send(r, p, &hdr);
}

static int
r2v_rdp_recv_confirm_active(r2v_rdp_t *r, r2v_packet_t *p)
{
	share_data_hdr_t hdr;

	if (r2v_rdp_recv(r, p, &hdr) == -1) {
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
r2v_rdp_recv_synchronize(r2v_rdp_t *r, r2v_packet_t *p)
{
	share_data_hdr_t hdr;

	if (r2v_rdp_recv(r, p, &hdr) == -1) {
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
r2v_rdp_recv_control_cooperate(r2v_rdp_t *r, r2v_packet_t *p)
{
	uint16_t action = 0;
	share_data_hdr_t hdr;

	if (r2v_rdp_recv(r, p, &hdr) == -1) {
		goto fail;
	}
	if (hdr.share_ctrl_hdr.type != PDUTYPE_DATAPDU ||
		hdr.pdu_type2 != PDUTYPE2_CONTROL) {
		goto fail;
	}
	/* action */
	R2V_PACKET_READ_UINT16_LE(p, action);
	if (action != CTRLACTION_COOPERATE) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
r2v_rdp_recv_control_request(r2v_rdp_t *r, r2v_packet_t *p)
{
	uint16_t action = 0;
	share_data_hdr_t hdr;

	if (r2v_rdp_recv(r, p, &hdr) == -1) {
		goto fail;
	}
	if (hdr.share_ctrl_hdr.type != PDUTYPE_DATAPDU ||
		hdr.pdu_type2 != PDUTYPE2_CONTROL) {
		goto fail;
	}
	/* action */
	R2V_PACKET_READ_UINT16_LE(p, action);
	if (action != CTRLACTION_REQUEST_CONTROL) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
r2v_rdp_recv_font_list(r2v_rdp_t *r, r2v_packet_t *p)
{
	share_data_hdr_t hdr;

	if (r2v_rdp_recv(r, p, &hdr) == -1) {
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
r2v_rdp_send_synchronize(r2v_rdp_t *r, r2v_packet_t *p)
{
	share_data_hdr_t hdr;

	r2v_rdp_init_packet(p, sizeof(share_data_hdr_t));
	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_SYNCHRONIZE;
	/* messageType */
	R2V_PACKET_WRITE_UINT16_LE(p, SYNCMSGTYPE_SYNC);
	/* targetUser */
	R2V_PACKET_WRITE_UINT16_LE(p, MCS_IO_CHANNEL_ID);

	R2V_PACKET_END(p);
	return r2v_rdp_send(r, p, &hdr);
}

static int
r2v_rdp_send_control_cooperate(r2v_rdp_t *r, r2v_packet_t *p)
{
	share_data_hdr_t hdr;

	r2v_rdp_init_packet(p, sizeof(share_data_hdr_t));
	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_CONTROL;
	/* action */
	R2V_PACKET_WRITE_UINT16_LE(p, CTRLACTION_COOPERATE);
	/* grantId */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* controlId */
	R2V_PACKET_WRITE_UINT32_LE(p, 0);

	R2V_PACKET_END(p);
	return r2v_rdp_send(r, p, &hdr);
}

static int
r2v_rdp_send_control_grant_control(r2v_rdp_t *r, r2v_packet_t *p)
{
	share_data_hdr_t hdr;

	r2v_rdp_init_packet(p, sizeof(share_data_hdr_t));
	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_CONTROL;
	/* action */
	R2V_PACKET_WRITE_UINT16_LE(p, CTRLACTION_GRANTED_CONTROL);
	/* grantId */
	R2V_PACKET_WRITE_UINT16_LE(p, r->sec->mcs->user_channel_id);
	/* controlId */
	R2V_PACKET_WRITE_UINT32_LE(p, 0x03EA);

	R2V_PACKET_END(p);
	return r2v_rdp_send(r, p, &hdr);
}

static int
r2v_rdp_send_font_map(r2v_rdp_t *r, r2v_packet_t *p)
{
	share_data_hdr_t hdr;

	r2v_rdp_init_packet(p, sizeof(share_data_hdr_t));
	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_FONTMAP;
	/* numberEntries */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* totalNumEntries */
	R2V_PACKET_WRITE_UINT16_LE(p, 0);
	/* mapFlags */
	R2V_PACKET_WRITE_UINT16_LE(p, FONTMAP_FIRST|FONTMAP_LAST);
	/* entrySize */
	R2V_PACKET_WRITE_UINT16_LE(p, 0x0004);

	R2V_PACKET_END(p);
	return r2v_rdp_send(r, p, &hdr);
}

static int
r2v_rdp_build_conn(r2v_rdp_t *r)
{
	r2v_packet_t *p = NULL;

	p = r2v_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	if (r2v_rdp_recv_client_info(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_send_license_error(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_send_demand_active(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_recv_confirm_active(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_recv_synchronize(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_recv_control_cooperate(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_recv_control_request(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_recv_font_list(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_send_synchronize(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_send_control_cooperate(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_send_control_grant_control(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_send_font_map(r, p) == -1) {
		goto fail;
	}

	r2v_packet_destory(p);
	return 0;

fail:
	r2v_packet_destory(p);
	return -1;
}

r2v_rdp_t *
r2v_rdp_init(int client_fd, r2v_session_t *s)
{
	r2v_rdp_t *r = NULL;

	r = (r2v_rdp_t *)malloc(sizeof(r2v_rdp_t));
	if (r == NULL) {
		goto fail;
	}
	memset(r, 0, sizeof(r2v_rdp_t));

	r->session = s;

	r->packet = r2v_packet_init(65535);
	if (r->packet == NULL) {
		goto fail;
	}

	r->sec = r2v_sec_init(client_fd);
	if (r->sec == NULL) {
		goto fail;
	}

	if (r2v_rdp_build_conn(r) == -1) {
		goto fail;
	}

	return r;

fail:
	r2v_rdp_destory(r);
	return NULL;
}

void
r2v_rdp_destory(r2v_rdp_t *r)
{
	if (r == NULL) {
		return;
	}
	if (r->packet != NULL) {
		r2v_packet_destory(r->packet);
	}
	if (r->sec != NULL) {
		r2v_sec_destory(r->sec);
	}
	free(r);
}

void
r2v_rdp_init_packet(r2v_packet_t *p, uint16_t offset)
{
	r2v_mcs_init_packet(p);
	p->rdp = p->current;
	R2V_PACKET_SEEK(p, offset);
}

int
r2v_rdp_recv(r2v_rdp_t *r, r2v_packet_t *p, share_data_hdr_t *hdr)
{
	uint8_t choice;
	uint16_t channel_id;

	if (r2v_mcs_recv(r->sec->mcs, p, &choice, &channel_id) == -1) {
		goto fail;
	}
	if (!R2V_PACKET_READ_REMAIN(p, sizeof(share_ctrl_hdr_t))) {
		goto fail;
	}
	R2V_PACKET_READ_N(p, &(hdr->share_ctrl_hdr), sizeof(share_ctrl_hdr_t));
	if (hdr->share_ctrl_hdr.version_low != TS_PROTOCOL_VERSION ||
		hdr->share_ctrl_hdr.version_high != 0x00) {
		goto fail;
	}
	if (hdr->share_ctrl_hdr.type == PDUTYPE_DATAPDU) {
		R2V_PACKET_READ_N(p, (void *)hdr + sizeof(share_ctrl_hdr_t),
						  sizeof(share_data_hdr_t) - sizeof(share_ctrl_hdr_t));
	}

	return 0;

fail:
	return -1;
}

int
r2v_rdp_send(r2v_rdp_t *r, r2v_packet_t *p, share_data_hdr_t *hdr)
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
		R2V_PACKET_WRITE_N(p, hdr, sizeof(share_data_hdr_t));
	} else {
		R2V_PACKET_WRITE_N(p, &(hdr->share_ctrl_hdr), sizeof(share_ctrl_hdr_t));
	}

	return r2v_mcs_send(r->sec->mcs, p, MCS_SEND_DATA_INDICATION,
						MCS_IO_CHANNEL_ID);
}

int
r2v_rdp_send_bitmap_update(r2v_rdp_t *r, uint16_t left, uint16_t top,
						   uint16_t right, uint16_t bottom,
						   uint16_t width, uint16_t height,
						   uint16_t bpp, uint16_t bitmap_length,
						   uint8_t *bitmap_data)
{
	r2v_packet_t *p = NULL;
	share_data_hdr_t hdr;

	p = r2v_packet_init(bitmap_length + 100);
	if (p == NULL) {
		goto fail;
	}
	r2v_rdp_init_packet(p, sizeof(share_data_hdr_t));

	/* shareDataHeader */
	hdr.share_ctrl_hdr.type = PDUTYPE_DATAPDU;
	hdr.pdu_type2 = PDUTYPE2_UPDATE;
	/* updateType */
	R2V_PACKET_WRITE_UINT16_LE(p, UPDATETYPE_BITMAP);
	/* numberRectangles */
	R2V_PACKET_WRITE_UINT16_LE(p, 1);
	/* destLeft */
	R2V_PACKET_WRITE_UINT16_LE(p, left);
	/* destTop */
	R2V_PACKET_WRITE_UINT16_LE(p, top);
	/* destRight */
	R2V_PACKET_WRITE_UINT16_LE(p, right);
	/* destBottom */
	R2V_PACKET_WRITE_UINT16_LE(p, bottom);
	/* width */
	R2V_PACKET_WRITE_UINT16_LE(p, width);
	/* height */
	R2V_PACKET_WRITE_UINT16_LE(p, height);
	/* bitsPerPixel */
	R2V_PACKET_WRITE_UINT16_LE(p, bpp);
	/* flags */
	R2V_PACKET_WRITE_UINT16_LE(p, NO_BITMAP_COMPRESSION_HDR);
	/* bitmapLength */
	R2V_PACKET_WRITE_UINT16_LE(p, bitmap_length);
	/* bitmapDataStream */
	R2V_PACKET_WRITE_N(p, bitmap_data, bitmap_length);
	/* send packet */
	R2V_PACKET_END(p);
	if (r2v_rdp_send(r, p, &hdr) == -1) {
		goto fail;
	}

	r2v_packet_destory(p);
	return 0;

fail:
	r2v_packet_destory(p);
	return -1;
}

static int
r2v_rdp_process_data(r2v_rdp_t *r, r2v_packet_t *p, const share_data_hdr_t *hdr)
{
	switch (hdr->pdu_type2) {
	case PDUTYPE2_INPUT:
		if (r2v_input_process(r, p) == -1) {
			goto fail;
		}
		break;
	}

	return 0;

fail:
	return -1;
}

int
r2v_rdp_process(r2v_rdp_t *r)
{
	share_data_hdr_t hdr;

	if (r2v_rdp_recv(r, r->packet, &hdr) == -1) {
		goto fail;
	}
	switch (hdr.share_ctrl_hdr.type) {
	case PDUTYPE_DEMANDACTIVEPDU:
		break;
	case PDUTYPE_DATAPDU:
		if (r2v_rdp_process_data(r, r->packet, &hdr) == -1) {
			goto fail;
		}
		break;
	}

	return 0;

fail:
	return -1;
}
