#ifndef _RDP_CONN_H_
#define _RDP_CONN_H_

#include "sec.h"

typedef struct _r2v_rdp_conn_t {
	r2v_sec_t *sec;
} r2v_rdp_conn_t;

extern r2v_rdp_conn_t *r2v_rdp_conn_init(int client_fd);
extern void r2v_rdp_conn_destory(r2v_rdp_conn_t *c);

#endif
