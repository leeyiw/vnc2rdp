#ifndef _VNC_H_
#define _VNC_H_

#include "packet.h"
#include "session.h"

#define RFB_PROTOCOL_VERSION			"RFB 003.003\n"

typedef struct _r2v_vnc_t {
	int fd;
	r2v_packet_t *packet;

	r2v_session_t *session;
} r2v_vnc_t;

extern r2v_vnc_t *r2v_vnc_init();
extern void r2v_vnc_destory(r2v_vnc_t *v);

#endif
