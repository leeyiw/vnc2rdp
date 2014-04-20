#include <stdlib.h>
#include <string.h>

#include "packet.h"

packet_t *
r2v_packet_init(int max_len)
{
	packet_t *p = NULL;

	p = (packet_t *)malloc(sizeof(packet_t));
	if (p == NULL) {
		return NULL;
	}
	memset(p, 0, sizeof(packet_t));

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
r2v_packet_reset(packet_t *p)
{
	if (p == NULL) {
		return;
	}
	p->total_len = 0;
	memset(p->data, 0, p->max_len);
	p->current = p->data;
}

void
r2v_packet_destory(packet_t *p)
{
	if (p == NULL) {
		return;
	}
	if (p->data != NULL) {
		free(p->data);
	}
	free(p);
}

void
r2v_packet_end(packet_t *p)
{
	p->total_len = p->current - p->data;
}
