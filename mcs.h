/**
 * vnc2rdp: proxy for RDP client connect to VNC server
 *
 * Copyright 2014 Yiwei Li <leeyiw@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

typedef struct _v2r_mcs_t {
	v2r_x224_t *x224;

	/* client core data */
	uint32_t keyboard_layout;

	/* client network data */
	uint32_t channel_count;
	channel_def_t *channel_def_array;
	uint16_t user_channel_id;
} v2r_mcs_t;

extern v2r_mcs_t *v2r_mcs_init(int client_fd);
extern void v2r_mcs_destory(v2r_mcs_t *m);
extern int v2r_mcs_recv(v2r_mcs_t *m, v2r_packet_t *p, uint8_t *choice,
						uint16_t *channel_id);
extern int v2r_mcs_send(v2r_mcs_t *m, v2r_packet_t *p, uint8_t choice,
						uint16_t channel_id);
extern void v2r_mcs_init_packet(v2r_packet_t *p);

#endif
