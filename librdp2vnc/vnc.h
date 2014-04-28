#ifndef _VNC_H_
#define _VNC_H_

#include "packet.h"
#include "session.h"

#define RFB_PROTOCOL_VERSION			"RFB 003.003\n"

#define RFB_SEC_TYPE_NONE				1
#define RFB_SEC_TYPE_VNC_AUTH			2

typedef struct _r2v_vnc_t {
	int fd;
	r2v_packet_t *packet;

	uint32_t security_type;

	r2v_session_t *session;
} r2v_vnc_t;

extern r2v_vnc_t *r2v_vnc_init();
extern void r2v_vnc_destory(r2v_vnc_t *v);

#endif
