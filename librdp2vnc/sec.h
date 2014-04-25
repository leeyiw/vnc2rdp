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

#define SEC_EXCHANGE_PKT		0x0001
#define SEC_TRANSPORT_REQ		0x0002
#define RDP_SEC_TRANSPORT_RSP	0x0004
#define SEC_ENCRYPT				0x0008
#define SEC_RESET_SEQNO			0x0010
#define SEC_IGNORE_SEQNO		0x0020
#define SEC_INFO_PKT			0x0040
#define SEC_LICENSE_PKT			0x0080
#define SEC_LICENSE_ENCRYPT_CS	0x0200
#define SEC_LICENSE_ENCRYPT_SC	0x0200
#define SEC_REDIRECTION_PKT		0x0400
#define SEC_SECURE_CHECKSUM		0x0800
#define SEC_AUTODETECT_REQ		0x1000
#define SEC_AUTODETECT_RSP		0x2000
#define SEC_HEARTBEAT			0x4000
#define SEC_FLAGSHI_VALID		0x8000

typedef struct _r2v_sec_t {
	r2v_mcs_t *mcs;
} r2v_sec_t;

extern r2v_sec_t *r2v_sec_init(int client_fd);
extern void r2v_sec_destory(r2v_sec_t *s);
extern int r2v_sec_recv(r2v_sec_t *s, r2v_packet_t *p, uint16_t *sec_flags,
						uint16_t *channel_id);
extern int r2v_sec_send(r2v_sec_t *s, r2v_packet_t *p, uint16_t sec_flags,
						uint16_t channel_id);
extern void r2v_sec_init_packet(r2v_packet_t *p);

#endif
