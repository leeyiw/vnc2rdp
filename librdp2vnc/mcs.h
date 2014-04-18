#ifndef _MCS_H_
#define _MCS_H_

#include "x224.h"

typedef struct _r2v_mcs_t {
	r2v_x224_t *x224;
} r2v_mcs_t;

extern r2v_mcs_t *r2v_mcs_init(int client_fd);
extern void r2v_mcs_destory(r2v_mcs_t *m);

#endif
