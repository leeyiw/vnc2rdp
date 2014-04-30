#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
	R2V_PACKET_WRITE_UINT16_BE(v->packet, 4);
	R2V_PACKET_WRITE_SINT32_BE(v->packet, RFB_ENCODING_RAW);
	R2V_PACKET_WRITE_SINT32_BE(v->packet, RFB_ENCODING_COPYRECT);
	R2V_PACKET_WRITE_SINT32_BE(v->packet, RFB_ENCODING_CURSOR);
	R2V_PACKET_WRITE_SINT32_BE(v->packet, RFB_ENCODING_DESKTOP_SIZE);
	R2V_PACKET_END(v->packet);
	if (r2v_vnc_send(v) == -1) {
		goto fail;
	}

	/* send FramebufferUpdateRequest message */
	r2v_packet_reset(v->packet);
	R2V_PACKET_WRITE_UINT8(v->packet, RFB_FRAME_BUFFER_UPDATE_REQUEST);
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
