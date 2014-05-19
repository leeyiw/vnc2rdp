#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "rdp.h"
#include "vnc.h"

static int
r2v_vnc_recv(r2v_vnc_t *v)
{
	int n = 0;

	r2v_packet_reset(v->packet);

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
r2v_vnc_recv1(r2v_vnc_t *v, size_t len)
{
	int n = 0;

	r2v_packet_reset(v->packet);

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
r2v_vnc_send(r2v_vnc_t *v)
{
	int n = 0;

	n = send(v->fd, v->packet->data, R2V_PACKET_LEN(v->packet), 0);
	if (n == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
r2v_vnc_build_conn(r2v_vnc_t *v)
{
	/* receive ProtocolVersion */
	if (r2v_vnc_recv(v) == -1) {
		goto fail;
	}

	/* send ProtocolVersion */
	r2v_packet_reset(v->packet);
	R2V_PACKET_WRITE_N(v->packet, RFB_PROTOCOL_VERSION,
					   strlen(RFB_PROTOCOL_VERSION));
	R2V_PACKET_END(v->packet);
	if (r2v_vnc_send(v) == -1) {
		goto fail;
	}

	/* receive security-type */
	if (r2v_vnc_recv(v) == -1) {
		goto fail;
	}
	R2V_PACKET_READ_UINT32_BE(v->packet, v->security_type);
	switch (v->security_type) {
	case RFB_SEC_TYPE_NONE:
		break;
	default:
		goto fail;
	}

	/* send ClientInit message */
	r2v_packet_reset(v->packet);
	R2V_PACKET_WRITE_UINT8(v->packet, 1);
	R2V_PACKET_END(v->packet);
	if (r2v_vnc_send(v) == -1) {
		goto fail;
	}

	/* recv ServerInit message */
	if (r2v_vnc_recv(v) == -1) {
		goto fail;
	}
	R2V_PACKET_READ_UINT16_BE(v->packet, v->framebuffer_width);
	R2V_PACKET_READ_UINT16_BE(v->packet, v->framebuffer_height);

	/* send SetPixelFormat message */
	r2v_packet_reset(v->packet);
	R2V_PACKET_WRITE_UINT8(v->packet, RFB_SET_PIXEL_FORMAT);
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	/* bits-per-pixel */
	R2V_PACKET_WRITE_UINT8(v->packet, 32);
	/* depth */
	R2V_PACKET_WRITE_UINT8(v->packet, 24);
	/* big-endian-flag */
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	/* true-colour-flag */
	R2V_PACKET_WRITE_UINT8(v->packet, 1);
	/* red-max */
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 255);
	/* green-max */
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 255);
	/* blue-max */
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 255);
	/* red-shift */
	R2V_PACKET_WRITE_UINT8(v->packet, 16);
	/* green-shift */
	R2V_PACKET_WRITE_UINT8(v->packet, 8);
	/* blue-shift */
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	/* padding */
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	R2V_PACKET_END(v->packet);
	if (r2v_vnc_send(v) == -1) {
		goto fail;
	}

	/* send SetEncodings message */
	r2v_packet_reset(v->packet);
	R2V_PACKET_WRITE_UINT8(v->packet, RFB_SET_ENCODINGS);
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 3);
	R2V_PACKET_WRITE_SINT32_BE(v->packet, RFB_ENCODING_RAW);
	R2V_PACKET_WRITE_SINT32_BE(v->packet, RFB_ENCODING_COPYRECT);
	//R2V_PACKET_WRITE_SINT32_BE(v->packet, RFB_ENCODING_CURSOR);
	R2V_PACKET_WRITE_SINT32_BE(v->packet, RFB_ENCODING_DESKTOP_SIZE);
	R2V_PACKET_END(v->packet);
	if (r2v_vnc_send(v) == -1) {
		goto fail;
	}

	/* send FramebufferUpdateRequest message */
	r2v_packet_reset(v->packet);
	R2V_PACKET_WRITE_UINT8(v->packet, RFB_FRAMEBUFFER_UPDATE_REQUEST);
	/* incremental */
	R2V_PACKET_WRITE_UINT8(v->packet, 0);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 0);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 0);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, v->framebuffer_width);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, v->framebuffer_height);
	R2V_PACKET_END(v->packet);
	if (r2v_vnc_send(v) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

r2v_vnc_t *
r2v_vnc_init(int server_fd)
{
	r2v_vnc_t *v = NULL;

	v = (r2v_vnc_t *)malloc(sizeof(r2v_vnc_t));
	if (v == NULL) {
		goto fail;
	}
	memset(v, 0, sizeof(r2v_vnc_t));

	v->fd = server_fd;

	v->packet = r2v_packet_init(65535);
	if (v->packet == NULL) {
		goto fail;
	}

	if (r2v_vnc_build_conn(v) == -1) {
		goto fail;
	}

	return v;

fail:
	r2v_vnc_destory(v);
	return NULL;
}

void
r2v_vnc_destory(r2v_vnc_t *v)
{
	if (v == NULL) {
		return;
	}
	if (v->fd != 0) {
		close(v->fd);
	}
	if (v->packet != NULL) {
		r2v_packet_destory(v->packet);
	}
	free(v);
}

static int
r2v_vnc_process_raw_encoding(r2v_vnc_t *v, uint16_t x, uint16_t y,
							 uint16_t w, uint16_t h)
{
	uint16_t left, top, right, bottom, width, height, i;
	uint32_t data_size = w * h * 4;
	uint32_t line_size = w * 4;
	uint8_t buffer[line_size];

	/* if data size is larger than vnc packet's buffer, 
	 * init a new packet with a larger buffer */
	if (data_size > v->packet->max_len) {
		r2v_packet_destory(v->packet);
		v->packet = r2v_packet_init(data_size);
		if (v->packet == NULL) {
			goto fail;
		}
	}
	if (r2v_vnc_recv1(v, data_size) == -1) {
		goto fail;
	}

	for (i = 0; i < h / 2; i++) {
		memcpy(buffer, v->packet->data + i * line_size, line_size);
		memcpy(v->packet->data + i * line_size,
			   v->packet->data + (h - i - 1) * line_size,
			   line_size);
		memcpy(v->packet->data + (h - i - 1) * line_size, buffer, line_size);
	}

/*
	left = x;
	top = y;
	right = x + w - 1;
	bottom = y + h - 1;
	width = w;
	height = h;
	if (r2v_rdp_send_bitmap_update(v->session->rdp, left, top, right, bottom,
								   width, height, 32, data_size,
								   v->packet->data) == -1) {
		goto fail;
	}
*/

	for (i = 0; i < h; i++) {
		left = x;
		top = y + h - 1 - i;
		right = x + w - 1;
		bottom = y + h - 1 - i;
		width = w;
		height = 1;
		if (r2v_rdp_send_bitmap_update(v->session->rdp, left, top, right, bottom,
									   width, height, 32, line_size,
									   v->packet->data + i * line_size) == -1) {
			goto fail;
		}
	}

	return 0;

fail:
	return -1;
}

static int
r2v_vnc_process_copy_rect_encoding(r2v_vnc_t *v, uint16_t x, uint16_t y,
								   uint16_t w, uint16_t h)
{
	uint16_t src_x, src_y;

	if (r2v_vnc_recv1(v, 4) == -1) {
		goto fail;
	}
	R2V_PACKET_READ_UINT16_BE(v->packet, src_x);
	R2V_PACKET_READ_UINT16_BE(v->packet, src_y);
	r2v_log_debug("copy rect from src_x: %d src_y: %d", src_x, src_y);

	return 0;

fail:
	return -1;
}

static int
r2v_vnc_process_framebuffer_update(r2v_vnc_t *v)
{
	uint16_t nrects = 0, i = 0, x, y, w, h;
	int32_t encoding_type;

	if (r2v_vnc_recv1(v, 3) == -1) {
		goto fail;
	}
	R2V_PACKET_SEEK_UINT8(v->packet);
	R2V_PACKET_READ_UINT16_BE(v->packet, nrects);
	r2v_log_debug("receive framebuffer update with %d rects", nrects);

	for (i = 0; i < nrects; i++) {
		if (r2v_vnc_recv1(v, 12) == -1) {
			goto fail;
		}
		R2V_PACKET_READ_UINT16_BE(v->packet, x);
		R2V_PACKET_READ_UINT16_BE(v->packet, y);
		R2V_PACKET_READ_UINT16_BE(v->packet, w);
		R2V_PACKET_READ_UINT16_BE(v->packet, h);
		R2V_PACKET_READ_UINT32_BE(v->packet, encoding_type);
		r2v_log_debug("rect %d of %d: pos: %d,%d size: %dx%d encoding: %d",
					  i + 1, nrects, x, y, w, h, encoding_type);

		switch (encoding_type) {
		case RFB_ENCODING_RAW:
			if (r2v_vnc_process_raw_encoding(v, x, y, w, h) == -1) {
				goto fail;
			}
			break;
		case RFB_ENCODING_COPYRECT:
			if (r2v_vnc_process_copy_rect_encoding(v, x, y, w, h) == -1) {
				goto fail;
			}
			break;
		default:
			r2v_log_warn("unknown encoding type: %d", encoding_type);
			break;
		}
	}

	/* send FramebufferUpdateRequest message */
	r2v_packet_reset(v->packet);
	R2V_PACKET_WRITE_UINT8(v->packet, RFB_FRAMEBUFFER_UPDATE_REQUEST);
	/* incremental */
	R2V_PACKET_WRITE_UINT8(v->packet, 1);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 0);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 0);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, v->framebuffer_width);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, v->framebuffer_height);
	R2V_PACKET_END(v->packet);
	if (r2v_vnc_send(v) == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

int
r2v_vnc_process(r2v_vnc_t *v)
{
	uint8_t msg_type;

	if (r2v_vnc_recv1(v, 1) == -1) {
		goto fail;
	}
	R2V_PACKET_READ_UINT8(v->packet, msg_type);

	switch (msg_type) {
	case RFB_FRAMEBUFFER_UPDATE:
		if (r2v_vnc_process_framebuffer_update(v) == -1) {
			goto fail;
		}
		break;
	default:
		r2v_log_debug("reveive unknown message type %d from vnc server",
					  msg_type);
		goto fail;
		break;
	}

	return 0;

fail:
	return -1;
}

int
r2v_vnc_send_pointer_event(r2v_vnc_t *v, uint8_t btn_mask,
						   uint16_t x_pos, uint16_t y_pos)
{
	/* send PointerEvent message */
	r2v_packet_reset(v->packet);
	/* message-type */
	R2V_PACKET_WRITE_UINT8(v->packet, RFB_POINTER_EVENT);
	/* button-mask */
	R2V_PACKET_WRITE_UINT8(v->packet, btn_mask);
	/* x-position */
	R2V_PACKET_WRITE_UINT16_BE(v->packet, x_pos);
	R2V_PACKET_WRITE_UINT16_BE(v->packet, y_pos);
	R2V_PACKET_END(v->packet);

	return r2v_vnc_send(v);
}
