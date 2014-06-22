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

#include <stdlib.h>
#include <string.h>

#include "sec.h"

static int
v2r_sec_build_conn(int client_fd, v2r_sec_t *s)
{
	return 0;
}

v2r_sec_t *
v2r_sec_init(int client_fd)
{
	v2r_sec_t *s = NULL;

	s = (v2r_sec_t *)malloc(sizeof(v2r_sec_t));
	if (s == NULL) {
		goto fail;
	}
	memset(s, 0, sizeof(v2r_sec_t));

	s->mcs = v2r_mcs_init(client_fd);
	if (s->mcs == NULL) {
		goto fail;
	}

	if (v2r_sec_build_conn(client_fd, s) == -1) {
		goto fail;
	}

	return s;

fail:
	v2r_sec_destory(s);
	return NULL;
}

void
v2r_sec_destory(v2r_sec_t *s)
{
	if (s == NULL) {
		return;
	}
	if (s->mcs != NULL) {
		v2r_mcs_destory(s->mcs);
	}
	free(s);
}

int
v2r_sec_recv(v2r_sec_t *s, v2r_packet_t *p, uint16_t *sec_flags,
			 uint16_t *channel_id)
{
	uint8_t choice = 0;

	if (v2r_mcs_recv(s->mcs, p, &choice, channel_id) == -1) {
		goto fail;
	}
	/* check if is send data request */
	if (choice != MCS_SEND_DATA_REQUEST) {
		goto fail;
	}
	/* parse security header */
	V2R_PACKET_READ_UINT16_LE(p, *sec_flags);
	/* skip flagsHi */
	V2R_PACKET_SEEK(p, 2);

	return 0;

fail:
	return -1;
}

int
v2r_sec_send(v2r_sec_t *s, v2r_packet_t *p, uint16_t sec_flags,
			 uint16_t channel_id)
{
	p->current = p->sec;
	V2R_PACKET_WRITE_UINT16_LE(p, sec_flags);
	V2R_PACKET_WRITE_UINT16_LE(p, 0);
	return v2r_mcs_send(s->mcs, p, MCS_SEND_DATA_INDICATION, channel_id);
}

void
v2r_sec_init_packet(v2r_packet_t *p)
{
	v2r_mcs_init_packet(p);
	p->sec = p->current;
	V2R_PACKET_SEEK(p, 4);
}
