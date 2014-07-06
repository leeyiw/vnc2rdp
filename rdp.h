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

#ifndef _RDP_H_
#define _RDP_H_

#include "keymaps.h"
#include "sec.h"
#include "session.h"

#define TS_PROTOCOL_VERSION						0x1

/* Share Control Header - pduType */
#define PDUTYPE_DEMANDACTIVEPDU					0x1
#define PDUTYPE_CONFIRMACTIVEPDU				0x3
#define PDUTYPE_DEACTIVATEALLPDU				0x6
#define PDUTYPE_DATAPDU							0x7
#define PDUTYPE_SERVER_REDIR_PKT				0xA

/* Share Data Header - streamId */
#define STREAM_UNDEFINED						0x00
#define STREAM_LOW								0x01
#define STREAM_MED								0x02
#define STREAM_HI								0x04

/* Share Data Header - pduType2 */
#define PDUTYPE2_UPDATE							0x02
#define PDUTYPE2_CONTROL						0x14
#define PDUTYPE2_POINTER						0x1B
#define PDUTYPE2_INPUT							0x1C
#define PDUTYPE2_SYNCHRONIZE					0x1F
#define PDUTYPE2_REFRESH_RECT					0x21
#define PDUTYPE2_PLAY_SOUND						0x22
#define PDUTYPE2_SUPPRESS_OUTPUT				0x23
#define PDUTYPE2_SHUTDOWN_REQUEST				0x24
#define PDUTYPE2_SHUTDOWN_DENIED				0x25
#define PDUTYPE2_SAVE_SESSION_INFO				0x26
#define PDUTYPE2_FONTLIST						0x27
#define PDUTYPE2_FONTMAP						0x28
#define PDUTYPE2_SET_KEYBOARD_INDICATORS		0x29
#define PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST	0x2B
#define PDUTYPE2_BITMAPCACHE_ERROR_PDU			0x2C
#define PDUTYPE2_SET_KEYBOARD_IME_STATUS		0x2D
#define PDUTYPE2_OFFSCRCACHE_ERROR_PDU			0x2E
#define PDUTYPE2_SET_ERROR_INFO_PDU				0x2F
#define PDUTYPE2_DRAWNINEGRID_ERROR_PDU			0x30
#define PDUTYPE2_DRAWGDIPLUS_ERROR_PDU			0x31
#define PDUTYPE2_ARC_STATUS_PDU					0x32
#define PDUTYPE2_STATUS_INFO_PDU				0x36
#define PDUTYPE2_MONITOR_LAYOUT_PDU				0x37

/* Synchronize PDU Data - messageType */
#define SYNCMSGTYPE_SYNC						0x0001

/* Control PDU Data - action */
#define CTRLACTION_REQUEST_CONTROL				0x0001
#define CTRLACTION_GRANTED_CONTROL				0x0002
#define CTRLACTION_DETACH						0x0003
#define CTRLACTION_COOPERATE					0x0004

/* Font Map PDU Data - mapFlags */
#define FONTMAP_FIRST							0x0001
#define FONTMAP_LAST							0x0002

/* Server Graphics Update PDU - updateType */
#define UPDATETYPE_ORDERS						0x0000
#define UPDATETYPE_BITMAP						0x0001
#define UPDATETYPE_PALETTE						0x0002
#define UPDATETYPE_SYNCHRONIZE					0x0003

/* Bitmap Data - flags */
#define BITMAP_COMPRESSION						0x0001
#define NO_BITMAP_COMPRESSION_HDR				0x0400

/* Suppress Output PDU Data - allowDisplayUpdates */
#define SUPPRESS_DISPLAY_UPDATES				0x00
#define ALLOW_DISPLAY_UPDATES					0x01

typedef struct _v2r_rdp_t {
	v2r_packet_t *packet;
	v2r_sec_t *sec;

	uint8_t allow_display_updates;

	/* keyboard input variables */
	v2r_keymap_t *keymap;
	uint8_t lshift;
	uint8_t rshift;
	uint8_t capslock;
	uint8_t numlock;
	uint8_t altgr;

	v2r_session_t *session;
} v2r_rdp_t;

typedef struct _share_ctrl_hdr_t {
	uint16_t total_length;
	uint16_t type:4;
	uint16_t version_low:4;
	uint16_t version_high:8;
	uint16_t pdu_source;
} __attribute__ ((packed)) share_ctrl_hdr_t;

typedef struct _share_data_hdr_t {
	share_ctrl_hdr_t share_ctrl_hdr;
	uint32_t share_id;
	uint8_t pad1;
	uint8_t stream_id;
	uint16_t uncompressed_length;
	uint8_t pdu_type2;
	uint8_t compressed_type;
	uint16_t compressed_length;
} __attribute__ ((packed)) share_data_hdr_t;

extern v2r_rdp_t *v2r_rdp_init(int client_fd, v2r_session_t *s);
extern void v2r_rdp_destory(v2r_rdp_t *r);
extern void v2r_rdp_init_packet(v2r_packet_t *p, uint16_t offset);
extern int v2r_rdp_recv(v2r_rdp_t *r, v2r_packet_t *p, share_data_hdr_t *hdr);
extern int v2r_rdp_send(v2r_rdp_t *r, v2r_packet_t *p, share_data_hdr_t *hdr);
extern int v2r_rdp_send_bitmap_update(v2r_rdp_t *r, uint16_t left, uint16_t top,
									  uint16_t right, uint16_t bottom,
									  uint16_t width, uint16_t height,
									  uint16_t bpp, uint16_t bitmap_length,
									  uint8_t *data);
extern int v2r_rdp_send_palette_update(v2r_rdp_t *r, uint32_t number_colors,
									   uint8_t (*palette_entries)[3]);
extern int v2r_rdp_send_scrblt_order(v2r_rdp_t *r, uint16_t left, uint16_t top,
									 uint16_t width, uint16_t height,
									 uint16_t x_src, uint16_t y_src);
extern int v2r_rdp_process(v2r_rdp_t *r);

#endif  // _RDP_H_
