#include <stdlib.h>
#include <string.h>

#include "sec.h"

static int
r2v_sec_build_conn(int client_fd, r2v_sec_t *s)
{
	return 0;
}

r2v_sec_t *
r2v_sec_init(int client_fd)
{
	r2v_sec_t *s = NULL;

	s = (r2v_sec_t *)malloc(sizeof(r2v_sec_t));
	if (s == NULL) {
		goto fail;
	}
	memset(s, 0, sizeof(r2v_sec_t));

	s->mcs = r2v_mcs_init(client_fd);
	if (s->mcs == NULL) {
		goto fail;
	}

	if (r2v_sec_build_conn(client_fd, s) == -1) {
		goto fail;
	}

	return s;

fail:
	r2v_sec_destory(s);
	return NULL;
}

void
r2v_sec_destory(r2v_sec_t *s)
{
	if (s == NULL) {
		return;
	}
	if (s->mcs != NULL) {
		r2v_mcs_destory(s->mcs);
	}
	free(s);
}

int
r2v_sec_recv(r2v_sec_t *s, packet_t *p, uint16_t *sec_flags)
{
	uint8_t choice = 0;
	uint16_t channel_id;

	if (r2v_mcs_recv(s->mcs, p, &choice, &channel_id) == -1) {
		goto fail;
	}
	/* check if is send data request */
	if (choice != MCS_SEND_DATA_REQUEST) {
		goto fail;
	}
	/* parse security header */
	R2V_PACKET_READ_UINT16_LE(p, *sec_flags);
	/* skip flagsHi */
	R2V_PACKET_SEEK(p, 2);

	return 0;

fail:
	return -1;
}
