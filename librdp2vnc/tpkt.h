#ifndef _TPKT_H_
#define _TPKT_H_

#include "packet.h"

#define TPKT_HEADER_LEN		4
#define TPKT_VERSION		3

extern int r2v_tpkt_recv_pkt(int client_fd, packet_t *p);
extern int r2v_tpkt_send_pkt(int client_fd, packet_t *p);
extern int r2v_tpkt_reset_pkt(packet_t *p);

#endif
