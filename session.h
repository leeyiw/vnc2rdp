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

#ifndef _SESSION_H_
#define _SESSION_H_

#include <netinet/in.h>

#define MAX_EVENTS					64

typedef struct _v2r_rdp_t v2r_rdp_t;
typedef struct _v2r_vnc_t v2r_vnc_t;

typedef struct _v2r_session_opt_t {
	char vnc_server_ip[INET_ADDRSTRLEN];
	uint16_t vnc_server_port;
	char vnc_password[8];
	uint8_t shared;
	uint8_t viewonly;
} v2r_session_opt_t;

typedef struct _v2r_session_t {
	v2r_rdp_t *rdp;
	v2r_vnc_t *vnc;
	int epoll_fd;
	const v2r_session_opt_t *opt;
} v2r_session_t;

extern v2r_session_t *v2r_session_init(int client_fd, int server_fd,
									   const v2r_session_opt_t *opt);
extern void v2r_session_destory(v2r_session_t *s);
extern void v2r_session_transmit(v2r_session_t *s);

#endif
