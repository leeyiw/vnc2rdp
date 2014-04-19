#ifndef _MCS_H_
#define _MCS_H_

#include "x224.h"

#define BER_TAG_CONNECT_INITIAL		0x7F65
#define BER_TAG_BOOLEAN				0x01
#define BER_TAG_OCTET_STRING		0x04
#define BER_TAG_DOMAIN_PARAMETERS	0x30

typedef struct _r2v_mcs_t {
	r2v_x224_t *x224;
} r2v_mcs_t;

extern r2v_mcs_t *r2v_mcs_init(int client_fd);
extern void r2v_mcs_destory(r2v_mcs_t *m);

#endif
