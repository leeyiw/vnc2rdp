#include <stdlib.h>
#include <string.h>

#include "packet.h"

r2v_packet_t *
r2v_packet_init(int max_len)
{
	r2v_packet_t *p = NULL;

	p = (r2v_packet_t *)malloc(sizeof(r2v_packet_t));
	if (p == NULL) {
		return NULL;
	}
	memset(p, 0, sizeof(r2v_packet_t));

	p->data = (uint8_t *)malloc(max_len);
	if (p->data == NULL) {
		goto fail;
	}
	p->max_len = max_len;
	p->current = p->data;

	return p;

fail:
	r2v_packet_destory(p);
	return NULL;
}

void
r2v_packet_reset(r2v_packet_t *p)
{
	if (p == NULL) {
		return;
	}
	memset(p->data, 0, p->max_len);
	p->current = p->data;
	p->end = p->data;
	p->tpkt = NULL;
	p->x224 = NULL;
	p->mcs = NULL;
	p->sec = NULL;
}

void
r2v_packet_destory(r2v_packet_t *p)
{
	if (p == NULL) {
		return;
	}
	if (p->data != NULL) {
		free(p->data);
	}
	free(p);
}
