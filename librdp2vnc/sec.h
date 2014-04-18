#ifndef _SEC_H_
#define _SEC_H_

#include "mcs.h"

typedef struct _r2v_sec_t {
	r2v_mcs_t *mcs;
} r2v_sec_t;

extern r2v_sec_t *r2v_sec_init(int client_fd);
extern void r2v_sec_destory(r2v_sec_t *s);

#endif
