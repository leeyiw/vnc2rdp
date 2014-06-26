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
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "rdp.h"
#include "vnc.h"
#include "vncauth.h"

static int
v2r_vnc_recv(v2r_vnc_t *v)
{
	int n = 0;

	v2r_packet_reset(v->packet);

	n = recv(v->fd, v->packet->current, v->packet->max_len, 0);
	if (n == -1 || n == 0) {
		goto fail;
	}
	v->packet->end += n;

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_recv1(v2r_vnc_t *v, size_t len)
{
	int n = 0;

	v2r_packet_reset(v->packet);

	n = recv(v->fd, v->packet->current, len, MSG_WAITALL);
	if (n == -1 || n == 0) {
		goto fail;
	}
	v->packet->end += n;

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_send(v2r_vnc_t *v)
{
	int n = 0;

	n = send(v->fd, v->packet->data, V2R_PACKET_LEN(v->packet), 0);
	if (n == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_process_vnc_authentication(v2r_vnc_t *v)
{
	uint8_t challenge[CHALLENGESIZE];
	uint32_t security_result;

	/* receive challenge */
	if (v2r_vnc_recv1(v, CHALLENGESIZE) == -1) {
		goto fail;
	}
	V2R_PACKET_READ_N(v->packet, challenge, CHALLENGESIZE);
	rfbEncryptBytes(challenge, v->session->opt->vnc_password);
	/* send response */
	v2r_packet_reset(v->packet);
	V2R_PACKET_WRITE_N(v->packet, challenge, CHALLENGESIZE);
	V2R_PACKET_END(v->packet);
	if (v2r_vnc_send(v) == -1) {
		goto fail;
	}
	/* receive SecurityResult */
	if (v2r_vnc_recv1(v, sizeof(security_result)) == -1) {
		goto fail;
	}
	V2R_PACKET_READ_UINT32_BE(v->packet, security_result);
	if (security_result == RFB_SEC_RESULT_OK) {
		v2r_log_info("vnc authentication success");
	} else {
		v2r_log_error("vnc authentication failed");
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_recv_server_init(v2r_vnc_t *v)
{
	if (v2r_vnc_recv(v) == -1) {
		goto fail;
	}
	V2R_PACKET_READ_UINT16_BE(v->packet, v->framebuffer_width);
	V2R_PACKET_READ_UINT16_BE(v->packet, v->framebuffer_height);
	V2R_PACKET_READ_UINT8(v->packet, v->bits_per_pixel);
	V2R_PACKET_READ_UINT8(v->packet, v->depth);
	V2R_PACKET_READ_UINT8(v->packet, v->big_endian_flag);
	V2R_PACKET_READ_UINT8(v->packet, v->true_colour_flag);
	V2R_PACKET_READ_UINT16_BE(v->packet, v->red_max);
	V2R_PACKET_READ_UINT16_BE(v->packet, v->green_max);
	V2R_PACKET_READ_UINT16_BE(v->packet, v->blue_max);
	V2R_PACKET_READ_UINT8(v->packet, v->red_shift);
	V2R_PACKET_READ_UINT8(v->packet, v->green_shift);
	V2R_PACKET_READ_UINT8(v->packet, v->blue_shift);
	v2r_log_info("server framebuffer size: %dx%d", v->framebuffer_width,
				 v->framebuffer_height);
	v2r_log_info("server bpp: %d, depth: %d, big_endian: %d, true_colour: %d, "
				 "red_max: %d, green_max: %d, blue_max: %d, "
				 "red_shift: %d, green_shift: %d, blue_shift: %d",
				 v->bits_per_pixel, v->depth, v->big_endian_flag,
				 v->true_colour_flag, v->red_max, v->green_max, v->blue_max,
				 v->red_shift, v->green_shift, v->blue_shift);

	/* set big_endian_flag to 0 because RDP use little endian */
	v->big_endian_flag = 0;

	/* set colour shift to RDP order */
	switch (v->depth) {
	case 24:
		v->red_shift = 16;
		v->green_shift = 8;
		v->blue_shift = 0;
		v->bpp = 32;
		break;
	case 16:
		v->red_shift = 11;
		v->green_shift = 5;
		v->blue_shift = 0;
		v->bpp = 16;
		break;
	case 15:
		v->red_shift = 10;
		v->green_shift = 5;
		v->blue_shift = 0;
		v->bpp = 15;
		break;
	case 8:
		v->bpp = 8;
		break;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_build_conn(v2r_vnc_t *v)
{
	/* receive ProtocolVersion */
	if (v2r_vnc_recv(v) == -1) {
		goto fail;
	}

	/* send ProtocolVersion */
	v2r_packet_reset(v->packet);
	V2R_PACKET_WRITE_N(v->packet, RFB_PROTOCOL_VERSION,
					   strlen(RFB_PROTOCOL_VERSION));
	V2R_PACKET_END(v->packet);
	if (v2r_vnc_send(v) == -1) {
		goto fail;
	}

	/* receive security-type */
	if (v2r_vnc_recv1(v, sizeof(v->security_type)) == -1) {
		goto fail;
	}
	V2R_PACKET_READ_UINT32_BE(v->packet, v->security_type);
	switch (v->security_type) {
	case RFB_SEC_TYPE_NONE:
		break;
	case RFB_SEC_TYPE_VNC_AUTH:
		if (v2r_vnc_process_vnc_authentication(v) == -1) {
			goto fail;
		}
		break;
	default:
		goto fail;
	}

	/* send ClientInit message */
	v2r_packet_reset(v->packet);
	V2R_PACKET_WRITE_UINT8(v->packet, v->session->opt->shared);
	v2r_log_info("connect with shared-flag: %d", v->session->opt->shared);
	V2R_PACKET_END(v->packet);
	if (v2r_vnc_send(v) == -1) {
		goto fail;
	}

	/* recv ServerInit message */
	if (v2r_vnc_recv_server_init(v) == -1) {
		goto fail;
	}

	/* send SetPixelFormat message */
	v2r_packet_reset(v->packet);
	V2R_PACKET_WRITE_UINT8(v->packet, RFB_SET_PIXEL_FORMAT);
	V2R_PACKET_WRITE_UINT8(v->packet, 0);
	V2R_PACKET_WRITE_UINT8(v->packet, 0);
	V2R_PACKET_WRITE_UINT8(v->packet, 0);
	/* bits-per-pixel */
	V2R_PACKET_WRITE_UINT8(v->packet, v->bits_per_pixel);
	/* depth */
	V2R_PACKET_WRITE_UINT8(v->packet, v->depth);
	/* big-endian-flag */
	V2R_PACKET_WRITE_UINT8(v->packet, v->big_endian_flag);
	/* true-colour-flag */
	V2R_PACKET_WRITE_UINT8(v->packet, v->true_colour_flag);
	/* red-max */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, v->red_max);
	/* green-max */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, v->green_max);
	/* blue-max */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, v->blue_max);
	/* red-shift */
	V2R_PACKET_WRITE_UINT8(v->packet, v->red_shift);
	/* green-shift */
	V2R_PACKET_WRITE_UINT8(v->packet, v->green_shift);
	/* blue-shift */
	V2R_PACKET_WRITE_UINT8(v->packet, v->blue_shift);
	/* padding */
	V2R_PACKET_WRITE_UINT8(v->packet, 0);
	V2R_PACKET_WRITE_UINT8(v->packet, 0);
	V2R_PACKET_WRITE_UINT8(v->packet, 0);
	V2R_PACKET_END(v->packet);
	if (v2r_vnc_send(v) == -1) {
		goto fail;
	}

	/* send SetEncodings message */
	v2r_packet_reset(v->packet);
	V2R_PACKET_WRITE_UINT8(v->packet, RFB_SET_ENCODINGS);
	V2R_PACKET_WRITE_UINT8(v->packet, 0);
	V2R_PACKET_WRITE_UINT16_BE(v->packet, 2);
	V2R_PACKET_WRITE_UINT32_BE(v->packet, RFB_ENCODING_RAW);
	V2R_PACKET_WRITE_UINT32_BE(v->packet, RFB_ENCODING_COPYRECT);
	//V2R_PACKET_WRITE_UINT32_BE(v->packet, RFB_ENCODING_CURSOR);
	//V2R_PACKET_WRITE_UINT32_BE(v->packet, RFB_ENCODING_DESKTOP_SIZE);
	V2R_PACKET_END(v->packet);
	if (v2r_vnc_send(v) == -1) {
		goto fail;
	}

	/* send FramebufferUpdateRequest message */
	if (v2r_vnc_send_fb_update_req(v, 0, 0, 0, v->framebuffer_width,
								   v->framebuffer_height) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

v2r_vnc_t *
v2r_vnc_init(int server_fd, v2r_session_t *s)
{
	v2r_vnc_t *v = NULL;

	v = (v2r_vnc_t *)malloc(sizeof(v2r_vnc_t));
	if (v == NULL) {
		goto fail;
	}
	memset(v, 0, sizeof(v2r_vnc_t));

	v->session = s;

	v->fd = server_fd;
	v->packet = v2r_packet_init(65535);
	if (v->packet == NULL) {
		goto fail;
	}
	v->buffer = NULL;
	v->buffer_size = 0;

	if (v2r_vnc_build_conn(v) == -1) {
		goto fail;
	}

	return v;

fail:
	v2r_vnc_destory(v);
	return NULL;
}

void
v2r_vnc_destory(v2r_vnc_t *v)
{
	if (v == NULL) {
		return;
	}
	if (v->fd != 0) {
		close(v->fd);
	}
	if (v->packet != NULL) {
		v2r_packet_destory(v->packet);
	}
	if (v->buffer != NULL) {
		free(v->buffer);
	}
	free(v);
}

static int
v2r_vnc_process_raw_encoding(v2r_vnc_t *v, uint16_t x, uint16_t y,
							 uint16_t w, uint16_t h)
{
	uint16_t left, top, right, bottom, width, height, i;
	uint32_t data_size, line_size, rdp_line_size, rdp_data_size;
	uint32_t max_line_per_packet, line_per_packet;
	const uint32_t max_byte_per_packet = 8192;

	/* VNC bitmap size */
	data_size = w * h * v->bits_per_pixel / 8;
	/* VNC bitmap row size */
	line_size = w * v->bits_per_pixel / 8;
	/* RDP bitmap row should contains a multiple of four bytes */
	rdp_line_size = (line_size % 4) ? ((line_size / 4) + 1) * 4 : line_size;
	/* RDP bitmap size */
	rdp_data_size = rdp_line_size * h;
	/* send at most max_line_per_packet line to RDP client in a RDP bitmap
	 * update PDU */
	max_line_per_packet = max_byte_per_packet / rdp_line_size;

	/* if data size is larger than vnc packet's buffer, 
	 * init a new packet with a larger buffer */
	if (data_size > v->packet->max_len) {
		v2r_packet_destory(v->packet);
		v->packet = v2r_packet_init(data_size);
		if (v->packet == NULL) {
			goto fail;
		}
	}
	/* and realloc a new buffer for VNC image upside down */
	if (rdp_data_size > v->buffer_size) {
		v->buffer_size = rdp_data_size;
		v->buffer = (uint8_t *)realloc(v->buffer, v->buffer_size);
		if (v->buffer == NULL) {
			v2r_log_error("failed to allocate memory for swap buffer");
			goto fail;
		}
	}

	/* receive VNC bitmap data */
	if (v2r_vnc_recv1(v, data_size) == -1) {
		goto fail;
	}

	for (i = 0; i < h; i++) {
		memcpy(v->buffer + (h - i - 1) * rdp_line_size,
			   v->packet->data + i * line_size,
			   line_size);
	}

/*
	left = x;
	top = y;
	right = x + w - 1;
	bottom = y + h - 1;
	width = w;
	height = h;
	if (v2r_rdp_send_bitmap_update(v->session->rdp, left, top, right, bottom,
								   width, height, 32, data_size,
								   v->packet->data) == -1) {
		goto fail;
	}
*/

	for (i = 0; i < h;) {
		if (i + max_line_per_packet > h) {
			line_per_packet = h - i;
		} else {
			line_per_packet = max_line_per_packet;
		}
		left = x;
		top = y + h - i - line_per_packet;
		right = x + w - 1;
		bottom = y + h - i - 1;
		width = w;
		height = line_per_packet;
		if (v2r_rdp_send_bitmap_update(v->session->rdp,
									   left, top, right, bottom,
									   width, height, v->bpp,
									   rdp_line_size * line_per_packet,
									   v->buffer + i * rdp_line_size) == -1) {
			goto fail;
		}
		i += line_per_packet;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_process_copy_rect_encoding(v2r_vnc_t *v, uint16_t x, uint16_t y,
								   uint16_t w, uint16_t h)
{
	uint16_t src_x, src_y;

	if (v2r_vnc_recv1(v, 4) == -1) {
		goto fail;
	}
	V2R_PACKET_READ_UINT16_BE(v->packet, src_x);
	V2R_PACKET_READ_UINT16_BE(v->packet, src_y);
	v2r_log_debug("copy rect from src_x: %d src_y: %d", src_x, src_y);

	if (v2r_rdp_send_scrblt_order(v->session->rdp, x, y, w, h, src_x, src_y)
		== -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_process_framebuffer_update(v2r_vnc_t *v)
{
	uint16_t nrects = 0, i = 0, x, y, w, h;
	int32_t encoding_type;

	if (v2r_vnc_recv1(v, 3) == -1) {
		goto fail;
	}
	V2R_PACKET_SEEK_UINT8(v->packet);
	V2R_PACKET_READ_UINT16_BE(v->packet, nrects);
	//v2r_log_debug("receive framebuffer update with %d rects", nrects);

	for (i = 0; i < nrects; i++) {
		if (v2r_vnc_recv1(v, 12) == -1) {
			goto fail;
		}
		V2R_PACKET_READ_UINT16_BE(v->packet, x);
		V2R_PACKET_READ_UINT16_BE(v->packet, y);
		V2R_PACKET_READ_UINT16_BE(v->packet, w);
		V2R_PACKET_READ_UINT16_BE(v->packet, h);
		V2R_PACKET_READ_UINT32_BE(v->packet, encoding_type);
		//v2r_log_debug("rect %d of %d: pos: %d,%d size: %dx%d encoding: %d",
		//			  i + 1, nrects, x, y, w, h, encoding_type);

		switch (encoding_type) {
		case RFB_ENCODING_RAW:
			if (v2r_vnc_process_raw_encoding(v, x, y, w, h) == -1) {
				goto fail;
			}
			break;
		case RFB_ENCODING_COPYRECT:
			if (v2r_vnc_process_copy_rect_encoding(v, x, y, w, h) == -1) {
				goto fail;
			}
			break;
		default:
			v2r_log_warn("unknown encoding type: %d", encoding_type);
			break;
		}
	}

	/* send FramebufferUpdateRequest message */
	if (v2r_vnc_send_fb_update_req(v, 1, 0, 0, v->framebuffer_width,
								   v->framebuffer_height) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_process_set_colour_map_entries(v2r_vnc_t *v)
{
	uint16_t i, first_colour, n_colours, red, green, blue;

	if (v2r_vnc_recv1(v, 5) == -1) {
		goto fail;
	}
	V2R_PACKET_SEEK(v->packet, 1);
	V2R_PACKET_READ_UINT16_BE(v->packet, first_colour);
	V2R_PACKET_READ_UINT16_BE(v->packet, n_colours);
	v2r_log_info("set colour map entries with first_colour: %d, n_colours: %d",
				 first_colour, n_colours);
	if (v2r_vnc_recv1(v, n_colours * 6) == -1) {
		goto fail;
	}
	for (i = 0; i < n_colours; i++) {
		V2R_PACKET_READ_UINT16_LE(v->packet, red);
		V2R_PACKET_READ_UINT16_LE(v->packet, green);
		V2R_PACKET_READ_UINT16_LE(v->packet, blue);
		v->colour_map[first_colour + i][0] = red;
		v->colour_map[first_colour + i][1] = green;
		v->colour_map[first_colour + i][2] = blue;
	}

	if (v2r_rdp_send_palette_update(v->session->rdp,
									RFB_COLOUR_MAP_ENTRIES_SIZE,
									v->colour_map) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
v2r_vnc_process_server_cut_text(v2r_vnc_t *v)
{
	uint32_t length;

	if (v2r_vnc_recv1(v, 7) == -1) {
		goto fail;
	}
	V2R_PACKET_SEEK(v->packet, 3);
	V2R_PACKET_READ_UINT32_BE(v->packet, length);
	if (v2r_vnc_recv1(v, length) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

int
v2r_vnc_process(v2r_vnc_t *v)
{
	uint8_t msg_type;

	if (v2r_vnc_recv1(v, 1) == -1) {
		goto fail;
	}
	V2R_PACKET_READ_UINT8(v->packet, msg_type);

	switch (msg_type) {
	case RFB_FRAMEBUFFER_UPDATE:
		if (v2r_vnc_process_framebuffer_update(v) == -1) {
			goto fail;
		}
		break;
	case RFB_SET_COLOUR_MAP_ENTRIES:
		if (v2r_vnc_process_set_colour_map_entries(v) == -1) {
			goto fail;
		}
		break;
	case RFB_SERVER_CUT_TEXT:
		if (v2r_vnc_process_server_cut_text(v) == -1) {
			goto fail;
		}
		break;
	default:
		v2r_log_debug("reveive unknown message type %d from vnc server",
					  msg_type);
		goto fail;
		break;
	}

	return 0;

fail:
	return -1;
}

int
v2r_vnc_send_fb_update_req(v2r_vnc_t *v, uint8_t incremental,
						   uint16_t x_pos, uint16_t y_pos,
						   uint16_t width, uint16_t height)
{
	/* if client sent Suppress Output PDU, then don't send framebuffer
	 * update request */
	if (v->session->rdp != NULL &&
		v->session->rdp->allow_display_updates == SUPPRESS_DISPLAY_UPDATES) {
		return 0;
	}

	/* send FramebufferUpdateRequest message */
	v2r_packet_reset(v->packet);

	/* message-type */
	V2R_PACKET_WRITE_UINT8(v->packet, RFB_FRAMEBUFFER_UPDATE_REQUEST);
	/* incremental */
	V2R_PACKET_WRITE_UINT8(v->packet, incremental);
	/* x-position */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, x_pos);
	/* y-position */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, y_pos);
	/* width */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, width);
	/* height */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, height);

	V2R_PACKET_END(v->packet);
	if (v2r_vnc_send(v) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

int
v2r_vnc_send_key_event(v2r_vnc_t *v, uint8_t down_flag, uint32_t key)
{
	/* if 'viewonly' is specified, don't send keyboard event to server */
	if (v->session->opt->viewonly) {
		return 0;
	}

	/* send KeyEvent message */
	v2r_packet_reset(v->packet);
	/* message-type */
	V2R_PACKET_WRITE_UINT8(v->packet, RFB_KEY_EVENT);
	/* down-flag */
	V2R_PACKET_WRITE_UINT8(v->packet, down_flag);
	/* padding */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, 0);
	/* key */
	V2R_PACKET_WRITE_UINT32_BE(v->packet, key);
	V2R_PACKET_END(v->packet);

	return v2r_vnc_send(v);
}

int
v2r_vnc_send_pointer_event(v2r_vnc_t *v, uint8_t btn_mask,
						   uint16_t x_pos, uint16_t y_pos)
{
	/* if 'viewonly' is specified, don't send pointer event to server */
	if (v->session->opt->viewonly) {
		return 0;
	}

	/* send PointerEvent message */
	v2r_packet_reset(v->packet);
	/* message-type */
	V2R_PACKET_WRITE_UINT8(v->packet, RFB_POINTER_EVENT);
	/* button-mask */
	V2R_PACKET_WRITE_UINT8(v->packet, btn_mask);
	/* x-position */
	V2R_PACKET_WRITE_UINT16_BE(v->packet, x_pos);
	V2R_PACKET_WRITE_UINT16_BE(v->packet, y_pos);
	V2R_PACKET_END(v->packet);

	return v2r_vnc_send(v);
}
