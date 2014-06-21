/**
 * rdp2vnc: proxy for RDP client connect to VNC server
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

#define MAX_EVENTS					64

typedef struct _r2v_rdp_t r2v_rdp_t;
typedef struct _r2v_vnc_t r2v_vnc_t;

typedef struct _r2v_session_t {
	r2v_rdp_t *rdp;
	r2v_vnc_t *vnc;
	int epoll_fd;
} r2v_session_t;

extern r2v_session_t *r2v_session_init(int client_fd, int server_fd,
									   const char *password);
extern void r2v_session_destory(r2v_session_t *s);
extern void r2v_session_transmit(r2v_session_t *s);

#endif
