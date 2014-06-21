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

#ifndef _PACKET_H_
#define _PACKET_H_

#include <stddef.h>
#include <stdint.h>

#define R2V_PACKET_READ_REMAIN(p, n) \
	((p)->current + n <= (p)->end)

#define R2V_PACKET_READ_UINT8(p, v) \
	do { \
		(v) = *(p)->current++; \
	} while (0)
#define R2V_PACKET_READ_UINT16_BE(p, v) \
	do { \
		(v) = *((uint8_t *)((p)->current++)) << 8; \
		(v) |= *((uint8_t *)((p)->current++)); \
	} while (0)
#define R2V_PACKET_READ_UINT16_LE(p, v) \
	do { \
		(v) = *((uint16_t *)((p)->current)); \
		(p)->current += sizeof(uint16_t); \
	} while (0)
#define R2V_PACKET_READ_UINT32_BE(p, v) \
	do { \
		(v) = (uint32_t) \
			( \
				(*((uint8_t *)((p)->current + 0)) << 24) | \
				(*((uint8_t *)((p)->current + 1)) << 16) | \
				(*((uint8_t *)((p)->current + 2)) << 8) | \
				(*((uint8_t *)((p)->current + 3)) << 0) \
			); \
		(p)->current += sizeof(uint32_t); \
	} while (0)
#define R2V_PACKET_READ_UINT32_LE(p, v) \
	do { \
		(v) = *((uint32_t *)((p)->current)); \
		(p)->current += sizeof(uint32_t); \
	} while (0)
#define R2V_PACKET_READ_N(p, v, n) \
	do { \
		memcpy((v), (p)->current, (n)); \
		(p)->current += n; \
	} while (0)

#define R2V_PACKET_WRITE_UINT8(p, v) \
	do { \
		*(p)->current++ = (uint8_t)v; \
	} while (0)
#define R2V_PACKET_WRITE_UINT16_BE(p, v) \
	do { \
		*((p)->current++) = (uint8_t)((v) >> 8); \
		*((p)->current++) = (uint8_t)((v) >> 0); \
	} while (0)
#define R2V_PACKET_WRITE_UINT16_LE(p, v) \
	do { \
		*((uint16_t *)((p)->current)) = (uint16_t)(v); \
		(p)->current += sizeof(uint16_t); \
	} while (0)
#define R2V_PACKET_WRITE_UINT32_BE(p, v) \
	do { \
		*((p)->current++) = (int8_t)((v) >> 24); \
		*((p)->current++) = (int8_t)((v) >> 16); \
		*((p)->current++) = (int8_t)((v) >> 8); \
		*((p)->current++) = (int8_t)((v) >> 0); \
	} while (0)
#define R2V_PACKET_WRITE_UINT32_LE(p, v) \
	do { \
		*((uint32_t *)((p)->current)) = (uint32_t)(v); \
		(p)->current += sizeof(uint32_t); \
	} while (0)
#define R2V_PACKET_WRITE_N(p, v, n) \
	do { \
		memcpy((p)->current, (v), (n)); \
		(p)->current += n; \
	} while (0)

#define R2V_PACKET_SEEK(p, n)			(p)->current += (n)
#define R2V_PACKET_SEEK_UINT8(p)		R2V_PACKET_SEEK(p, sizeof(uint8_t))
#define R2V_PACKET_SEEK_UINT16(p)		R2V_PACKET_SEEK(p, sizeof(uint16_t))

#define R2V_PACKET_END(p)				(p)->end = (p)->current
#define R2V_PACKET_LEN(p)				((p)->end - (p)->data)

typedef struct _r2v_packet_t {
	uint32_t max_len;
	uint8_t *data;
	uint8_t *current;
	uint8_t *end;
	/* RDP layer marks */
	uint8_t *tpkt;
	uint8_t *x224;
	uint8_t *mcs;
	uint8_t *sec;
	uint8_t *rdp;
} r2v_packet_t;

extern r2v_packet_t *r2v_packet_init(size_t max_len);
extern void r2v_packet_reset(r2v_packet_t *p);
extern void r2v_packet_destory(r2v_packet_t *p);

#endif  // _PACKET_H_
