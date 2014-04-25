#ifndef _MCS_H_
#define _MCS_H_

#include "x224.h"

#define BER_TAG_CONNECT_INITIAL			0x7F65
#define BER_TAG_CONNECT_RESPONSE		0x7F66
#define BER_TAG_BOOLEAN					0x01
#define BER_TAG_INTEGER					0x02
#define BER_TAG_OCTET_STRING			0x04
#define BER_TAG_ENUMERATED				0x0A
#define BER_TAG_DOMAIN_PARAMETERS		0x30

#define MCS_ERECT_DOMAIN_REQUEST		1
#define MCS_ATTACH_USER_REQUEST			10
#define MCS_ATTACH_USER_CONFIRM			11
#define MCS_CHANNEL_JOIN_REQUEST		14
#define MCS_CHANNEL_JOIN_CONFIRM		15
#define MCS_SEND_DATA_REQUEST			25
#define MCS_SEND_DATA_INDICATION		26

#define GCCCCRQ_HEADER_LEN				23
#define TS_UD_HEADER_LEN				4

#define CS_CORE							0xC001
#define CS_SECURITY						0xC002
#define CS_NET							0xC003
#define CS_CLUSTER						0xC004
#define SC_CORE							0x0C01
#define SC_SECURITY						0x0C02
#define SC_NET							0x0C03

#define MAX_CHANNELS_ALLOWED			31
#define MCS_BASE_CHANNEL_ID				1001
#define MCS_IO_CHANNEL_ID				1003

typedef struct _channel_def_t {
	char name[8];
	uint32_t options;
} channel_def_t;

typedef struct _r2v_mcs_t {
	r2v_x224_t *x224;

	/* client network data */
	uint32_t channel_count;
	channel_def_t *channel_def_array;
	uint16_t user_channel_id;
} r2v_mcs_t;

extern r2v_mcs_t *r2v_mcs_init(int client_fd);
extern void r2v_mcs_destory(r2v_mcs_t *m);
extern int r2v_mcs_recv(r2v_mcs_t *m, r2v_packet_t *p, uint8_t *choice,
						uint16_t *channel_id);
extern int r2v_mcs_send(r2v_mcs_t *m, r2v_packet_t *p, uint8_t choice,
						uint16_t channel_id);
extern void r2v_mcs_init_packet(r2v_packet_t *p);

#endif
