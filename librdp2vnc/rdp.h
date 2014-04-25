#ifndef _RDP_H_
#define _RDP_H_

#include "sec.h"

typedef struct _r2v_rdp_t {
	r2v_sec_t *sec;
} r2v_rdp_t;

extern r2v_rdp_t *r2v_rdp_init(int client_fd);
extern void r2v_rdp_destory(r2v_rdp_t *r);

#endif
