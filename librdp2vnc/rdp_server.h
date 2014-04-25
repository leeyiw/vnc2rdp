#ifndef _RDP_SERVER_H_
#define _RDP_SERVER_H_

#include <stdint.h>

#include "rdp.h"

typedef struct _r2v_rdp_server_t {
	int fd;
} r2v_rdp_server_t;

extern r2v_rdp_server_t *r2v_rdp_server_init(const char *ip, uint16_t port);
extern r2v_rdp_t *r2v_rdp_server_accept();

#endif
