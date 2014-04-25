#ifndef _TPKT_H_
#define _TPKT_H_

#include "packet.h"

#define TPKT_HEADER_LEN		4
#define TPKT_VERSION		3

typedef struct _r2v_tpkt_t {
	int fd;
} r2v_tpkt_t;

extern r2v_tpkt_t *r2v_tpkt_init(int client_fd);
extern void r2v_tpkt_destory(r2v_tpkt_t *t);
extern int r2v_tpkt_recv_pkt(r2v_tpkt_t *t, packet_t *p);
extern int r2v_tpkt_send_pkt(r2v_tpkt_t *t, packet_t *p);
extern int r2v_tpkt_reset_pkt(packet_t *p);

#endif
