#ifndef _RDP_H_
#define _RDP_H_

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

typedef struct _r2v_rdp_t {
	r2v_packet_t *packet;
	r2v_sec_t *sec;

	r2v_session_t *session;
} r2v_rdp_t;

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

extern r2v_rdp_t *r2v_rdp_init(int client_fd);
extern void r2v_rdp_destory(r2v_rdp_t *r);
extern void r2v_rdp_init_packet(r2v_packet_t *p, uint16_t offset);
extern int r2v_rdp_recv(r2v_rdp_t *r, r2v_packet_t *p, share_data_hdr_t *hdr);
extern int r2v_rdp_send(r2v_rdp_t *r, r2v_packet_t *p, share_data_hdr_t *hdr);
extern int r2v_rdp_send_bitmap_update(r2v_rdp_t *r, uint16_t left, uint16_t top,
									  uint16_t right, uint16_t bottom,
									  uint16_t width, uint16_t height,
									  uint16_t bpp, uint16_t bitmap_length,
									  uint8_t *data);
extern int r2v_rdp_process(r2v_rdp_t *r);

#endif
