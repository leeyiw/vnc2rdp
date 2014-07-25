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
#include <time.h>

#include "log.h"
#include "sec.h"

static const uint8_t pub_exp[4] = {0x01, 0x00, 0x01, 0x00};
static const uint8_t modulus[64] = {
	0x67, 0xab, 0x0e, 0x6a, 0x9f, 0xd6, 0x2b, 0xa3, 0x32, 0x2f,
	0x41, 0xd1, 0xce, 0xee, 0x61, 0xc3, 0x76, 0x0b, 0x26, 0x11,
	0x70, 0x48, 0x8a, 0x8d, 0x23, 0x81, 0x95, 0xa0, 0x39, 0xf7,
	0x5b, 0xaa, 0x3e, 0xf1, 0xed, 0xb8, 0xc4, 0xee, 0xce, 0x5f,
	0x6a, 0xf5, 0x43, 0xce, 0x5f, 0x60, 0xca, 0x6c, 0x06, 0x75,
	0xae, 0xc0, 0xd6, 0xa4, 0x0c, 0x92, 0xa4, 0xc6, 0x75, 0xea,
	0x64, 0xb2, 0x50, 0x5b
};
static const uint8_t modulus_zero_padding[8] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t signature[64] = {
	0x6a, 0x41, 0xb1, 0x43, 0xcf, 0x47, 0x6f, 0xf1, 0xe6, 0xcc,
	0xa1, 0x72, 0x97, 0xd9, 0xe1, 0x85, 0x15, 0xb3, 0xc2, 0x39,
	0xa0, 0xa6, 0x26, 0x1a, 0xb6, 0x49, 0x01, 0xfa, 0xa6, 0xda,
	0x60, 0xd7, 0x45, 0xf7, 0x2c, 0xee, 0xe4, 0x8e, 0x64, 0x2e,
	0x37, 0x49, 0xf0, 0x4c, 0x94, 0x6f, 0x08, 0xf5, 0x63, 0x4c,
	0x56, 0x29, 0x55, 0x5a, 0x63, 0x41, 0x2c, 0x20, 0x65, 0x95,
	0x99, 0xb1, 0x15, 0x7c
};
static const uint8_t signature_zero_padding[8] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
}

static int
v2r_sec_build_conn(int client_fd, v2r_sec_t *s)
{
	/* TODO receive Client Security Exchange PDU */
	return 0;
}

v2r_sec_t *
v2r_sec_init(int client_fd, v2r_session_t *session)
{
	v2r_sec_t *s = NULL;

	s = (v2r_sec_t *)malloc(sizeof(v2r_sec_t));
	if (s == NULL) {
		goto fail;
	}
	memset(s, 0, sizeof(v2r_sec_t));

	s->session = session;

	s->mcs = v2r_mcs_init(client_fd, session);
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

int
v2r_sec_generate_server_random(v2r_sec_t *s)
{
	int i;
	unsigned int seed;
	struct timespec tp;

	if (clock_gettime(CLOCK_MONOTONIC_COARSE, &tp) == -1) {
		v2r_log_error("get current time error: %s", ERRMSG);
		goto fail;
	}
	seed = (unsigned int)tp.tv_nsec;
	for (i = 0; i < SERVER_RANDOM_LEN; i++) {
		s->server_random[i] = rand_r(&seed);
	}

	return 0;

fail:
	return -1;
}

int
v2r_sec_write_server_certificate(v2r_sec_t *s, v2r_packet_t *p)
{
	/* Currently we only support server proprietary certificate */
	uint32_t keylen, bitlen, datalen;

	bitlen = 64 * 8;
	keylen = bitlen / 8 + 8;
	datalen = bitlen / 8 - 1;

	/* dwVersion */
	V2R_PACKET_WRITE_UINT32_LE(p, CERT_CHAIN_VERSION_1);
	/* dwSigAlgId */
	V2R_PACKET_WRITE_UINT32_LE(p, 0x00000001);
	/* dwKeyAlgId */
	V2R_PACKET_WRITE_UINT32_LE(p, 0x00000001);
	/* wPublicKeyBlobType */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0006);
	/* wPublicKeyBlobLen */
	V2R_PACKET_WRITE_UINT16_LE(p, 92);
	/* magic */
	V2R_PACKET_WRITE_UINT32_LE(p, 0x31415352);
	/* keylen */
	V2R_PACKET_WRITE_UINT32_LE(p, keylen);
	/* bitlen */
	V2R_PACKET_WRITE_UINT32_LE(p, bitlen);
	/* datalen */
	V2R_PACKET_WRITE_UINT32_LE(p, datalen);
	/* pubExp */
	V2R_PACKET_WRITE_N(p, pub_exp, sizeof(pub_exp));
	/* modulus */
	V2R_PACKET_WRITE_N(p, modulus, sizeof(modulus));
	/* 8 bytes of zero padding */
	V2R_PACKET_WRITE_N(p, modulus_zero_padding, sizeof(modulus_zero_padding));
	/* wSignatureBlobType */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0008);
	/* wSignatureBlobLen */
	V2R_PACKET_WRITE_UINT16_LE(p, 0x0048);
	/* SignatureBlob */
	V2R_PACKET_WRITE_N(p, signature, sizeof(signature));
	/* 8 bytes of zero padding */
	V2R_PACKET_WRITE_N(p, signature_zero_padding,
					   sizeof(signature_zero_padding));

	return 0;
}
