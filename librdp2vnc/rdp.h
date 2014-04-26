#ifndef _RDP_H_
#define _RDP_H_

#include "sec.h"

#define TS_PROTOCOL_VERSION			0x1

#define PDUTYPE_DEMANDACTIVEPDU		0x1
#define PDUTYPE_CONFIRMACTIVEPDU	0x3
#define PDUTYPE_DEACTIVATEALLPDU	0x6
#define PDUTYPE_DATAPDU				0x7
#define PDUTYPE_SERVER_REDIR_PKT	0xA

typedef struct _r2v_rdp_t {
	r2v_sec_t *sec;
} r2v_rdp_t;

typedef struct _share_control_header_t {
	uint16_t total_length;
	uint16_t type:4;
	uint16_t version_low:4;
	uint16_t version_high:8;
	uint16_t pdu_source;
} __attribute__ ((packed)) share_control_header_t;

extern r2v_rdp_t *r2v_rdp_init(int client_fd);
extern void r2v_rdp_destory(r2v_rdp_t *r);
extern void r2v_rdp_init_control_packet(r2v_packet_t *p);
extern void r2v_rdp_init_data_packet(r2v_packet_t *p);
extern int r2v_rdp_recv_control_packet(r2v_rdp_t *r, r2v_packet_t *p,
									   uint8_t *type);
extern int r2v_rdp_send_control_packet(r2v_rdp_t *r, r2v_packet_t *p,
									   uint8_t type);

#endif
