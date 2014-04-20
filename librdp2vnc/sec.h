#ifndef _SEC_H_
#define _SEC_H_

#include "mcs.h"

#define ENCRYPTION_METHOD_NONE				0x00000000
#define ENCRYPTION_METHOD_40BIT				0x00000001
#define ENCRYPTION_METHOD_128BIT			0x00000002
#define ENCRYPTION_METHOD_56BIT				0x00000008
#define ENCRYPTION_METHOD_FIPS				0x00000010

#define ENCRYPTION_LEVEL_NONE				0x00000000
#define ENCRYPTION_LEVEL_LOW				0x00000001
#define ENCRYPTION_LEVEL_CLIENT_COMPATIBLE	0x00000002
#define ENCRYPTION_LEVEL_HIGH				0x00000003
#define ENCRYPTION_LEVEL_FIPS				0x00000004

#define SERVER_RANDOM_LEN					32

typedef struct _r2v_sec_t {
	r2v_mcs_t *mcs;
} r2v_sec_t;

extern r2v_sec_t *r2v_sec_init(int client_fd);
extern void r2v_sec_destory(r2v_sec_t *s);

#endif
