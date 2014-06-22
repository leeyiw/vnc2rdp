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

#ifndef _X224_H_
#define _X224_H_

#define TPDU_CODE_CR				0xE0
#define TPDU_CODE_CC				0xD0
#define TPDU_CODE_DT				0xF0

#define X224_DATA_HEADER_LEN		3

#define TYPE_RDP_NEG_REQ			0x01
#define TYPE_RDP_NEG_RSP			0x02

#define X224_ROUTING_TOKEN_PREFIX	"Cookie: msts="
#define X224_COOKIE_PREFIX			"Cookie: mstshash="

#define PROTOCOL_RDP				0x00000000
#define PROTOCOL_SSL				0x00000001
#define PROTOCOL_HYBRID				0x00000002
#define PROTOCOL_HYBRID_EX			0x00000008

#include "tpkt.h"
#include "packet.h"

typedef struct _v2r_x224_t {
	v2r_tpkt_t *tpkt;

	uint32_t requested_protocols;
} v2r_x224_t;

extern v2r_x224_t *v2r_x224_init(int client_fd);
extern void v2r_x224_destory(v2r_x224_t *x);
extern int v2r_x224_recv(v2r_x224_t *x, v2r_packet_t *p);
extern int v2r_x224_send(v2r_x224_t *x, v2r_packet_t *p);
extern void v2r_x224_init_packet(v2r_packet_t *p);

#endif  // _X224_H_
