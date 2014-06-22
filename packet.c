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

#include "packet.h"

v2r_packet_t *
v2r_packet_init(size_t max_len)
{
	v2r_packet_t *p = NULL;

	p = (v2r_packet_t *)malloc(sizeof(v2r_packet_t));
	if (p == NULL) {
		return NULL;
	}
	memset(p, 0, sizeof(v2r_packet_t));

	p->data = (uint8_t *)malloc(max_len);
	if (p->data == NULL) {
		goto fail;
	}
	p->max_len = max_len;
	p->current = p->data;

	return p;

fail:
	v2r_packet_destory(p);
	return NULL;
}

void
v2r_packet_reset(v2r_packet_t *p)
{
	if (p == NULL) {
		return;
	}
	p->current = p->data;
	p->end = p->data;
	p->tpkt = NULL;
	p->x224 = NULL;
	p->mcs = NULL;
	p->sec = NULL;
}

void
v2r_packet_destory(v2r_packet_t *p)
{
	if (p == NULL) {
		return;
	}
	if (p->data != NULL) {
		free(p->data);
	}
	free(p);
}
