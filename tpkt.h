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

#ifndef _TPKT_H_
#define _TPKT_H_

#include "packet.h"

#define TPKT_HEADER_LEN		4
#define TPKT_VERSION		3

typedef struct _v2r_tpkt_t {
	int fd;
} v2r_tpkt_t;

extern v2r_tpkt_t *v2r_tpkt_init(int client_fd);
extern void v2r_tpkt_destory(v2r_tpkt_t *t);
extern int v2r_tpkt_recv(v2r_tpkt_t *t, v2r_packet_t *p);
extern int v2r_tpkt_send(v2r_tpkt_t *t, v2r_packet_t *p);
extern void v2r_tpkt_init_packet(v2r_packet_t *p);

#endif  // _TPKT_H_
