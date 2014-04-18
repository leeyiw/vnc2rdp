#ifndef _X224_H_
#define _X224_H_

#define TPDU_CODE_CR		0xE0
#define TPDU_CODE_CC		0xD0
#define TPDU_CODE_DT		0xF0

#define TYPE_RDP_NEG_REQ	0x01
#define TYPE_RDP_NEG_RSP	0x02

#define PROTOCOL_RDP		0x00000000
#define PROTOCOL_SSL		0x00000001
#define PROTOCOL_HYBRID		0x00000002
#define PROTOCOL_HYBRID_EX	0x00000008

#include "packet.h"

typedef struct _r2v_x224_t {
} r2v_x224_t;

extern r2v_x224_t *r2v_x224_init(int client_fd);
extern void r2v_x224_destory(r2v_x224_t *x);
extern int r2v_x224_recv_data_pkt(int client_fd, packet_t *p);

#endif
