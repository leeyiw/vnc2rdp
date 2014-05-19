#ifndef _VNC_H_
#define _VNC_H_

#include "packet.h"
#include "session.h"

#define RFB_PROTOCOL_VERSION			"RFB 003.003\n"

#define RFB_SEC_TYPE_NONE				1
#define RFB_SEC_TYPE_VNC_AUTH			2

/* client to server messages */
#define RFB_SET_PIXEL_FORMAT			0
#define RFB_SET_ENCODINGS				2
#define RFB_FRAMEBUFFER_UPDATE_REQUEST	3
#define RFB_KEY_EVENT					4
#define RFB_POINTER_EVENT				5
#define RFB_CLIENT_CUT_TEXT				6

/* PointerEvent */
#define RFB_POINTER_BUTTON_LEFT			1
#define RFB_POINTER_BUTTON_MIDDLE		2
#define RFB_POINTER_BUTTON_RIGHT		4

/* server to client messages */
#define RFB_FRAMEBUFFER_UPDATE			0
#define RFB_SET_COLOUR_MAP_ENTRIES		1
#define RFB_BELL						2
#define RFB_SERVER_CUT_TEXT				3

#define RFB_ENCODING_RAW				0
#define RFB_ENCODING_COPYRECT			1
#define RFB_ENCODING_RRE				2
#define RFB_ENCODING_HEXTILE			5
#define RFB_ENCODING_ZRLE				16
#define RFB_ENCODING_CURSOR				-239
#define RFB_ENCODING_DESKTOP_SIZE		-223

typedef struct _r2v_vnc_t {
	int fd;
	r2v_packet_t *packet;

	uint32_t security_type;
	uint16_t framebuffer_width;
	uint16_t framebuffer_height;

	r2v_session_t *session;
} r2v_vnc_t;

extern r2v_vnc_t *r2v_vnc_init();
extern void r2v_vnc_destory(r2v_vnc_t *v);
extern int r2v_vnc_process(r2v_vnc_t *v);
extern int r2v_vnc_send_pointer_event(r2v_vnc_t *v, uint8_t btn_mask,
									  uint16_t x_pos, uint16_t y_pos);

#endif
