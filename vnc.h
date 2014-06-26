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

#ifndef _VNC_H_
#define _VNC_H_

#include "packet.h"
#include "session.h"

#define RFB_PROTOCOL_VERSION			"RFB 003.003\n"

#define RFB_SEC_TYPE_NONE				1
#define RFB_SEC_TYPE_VNC_AUTH			2

/* SecurityResult */
#define RFB_SEC_RESULT_OK				0
#define RFB_SEC_RESULT_FAILED			1

/* client to server messages */
#define RFB_SET_PIXEL_FORMAT			0
#define RFB_SET_ENCODINGS				2
#define RFB_FRAMEBUFFER_UPDATE_REQUEST	3
#define RFB_KEY_EVENT					4
#define RFB_POINTER_EVENT				5
#define RFB_CLIENT_CUT_TEXT				6

/* PointerEvent */
#define RFB_POINTER_BUTTON_LEFT			0x01
#define RFB_POINTER_BUTTON_MIDDLE		0x02
#define RFB_POINTER_BUTTON_RIGHT		0x04
#define RFB_POINTER_WHEEL_UPWARDS		0x08
#define RFB_POINTER_WHEEL_DOWNWARDS		0x10

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

#define RFB_COLOUR_MAP_ENTRIES_SIZE		256

typedef struct _v2r_vnc_t {
	int fd;
	v2r_packet_t *packet;
	uint8_t *buffer;				/**< buffer for swap bitmap */
	uint32_t buffer_size;			/**< buffer size */

	uint32_t security_type;
	uint16_t framebuffer_width;
	uint16_t framebuffer_height;
	uint8_t bits_per_pixel;
	uint8_t depth;
	uint8_t big_endian_flag;
	uint8_t true_colour_flag;
	uint16_t red_max;
	uint16_t green_max;
	uint16_t blue_max;
	uint8_t red_shift;
	uint8_t green_shift;
	uint8_t blue_shift;

	uint16_t bpp;					/**< bits-per-pixel send to RDP client */
	uint8_t colour_map[RFB_COLOUR_MAP_ENTRIES_SIZE][3];

	v2r_session_t *session;
} v2r_vnc_t;

extern v2r_vnc_t *v2r_vnc_init(int server_fd, v2r_session_t *s);
extern void v2r_vnc_destory(v2r_vnc_t *v);
extern int v2r_vnc_process(v2r_vnc_t *v);
extern int v2r_vnc_send_fb_update_req(v2r_vnc_t *v, uint8_t incremental,
									  uint16_t x_pos, uint16_t y_pos,
									  uint16_t width, uint16_t height);
extern int v2r_vnc_send_key_event(v2r_vnc_t *v, uint8_t down_flag,
								  uint32_t key);
extern int v2r_vnc_send_pointer_event(v2r_vnc_t *v, uint8_t btn_mask,
									  uint16_t x_pos, uint16_t y_pos);

#endif  // _VNC_H_
