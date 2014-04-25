#include <stdlib.h>
#include <string.h>

#include "rdp.h"

static int
r2v_rdp_recv_client_info(r2v_rdp_t *r, packet_t *p)
{
	uint16_t sec_flags = 0;

	if (r2v_sec_recv(r->sec, p, &sec_flags) == -1) {
		goto fail;
	}
	if (sec_flags != SEC_INFO_PKT) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
r2v_rdp_send_license_error(r2v_rdp_t *r, packet_t *p)
{
	r2v_packet_reset(p);
	return 0;
}

static int
r2v_rdp_build_conn(r2v_rdp_t *r)
{
	packet_t *p = NULL;

	p = r2v_packet_init(8192);
	if (p == NULL) {
		goto fail;
	}

	if (r2v_rdp_recv_client_info(r, p) == -1) {
		goto fail;
	}
	if (r2v_rdp_send_license_error(r, p) == -1) {
		goto fail;
	}

	r2v_packet_destory(p);
	return 0;

fail:
	r2v_packet_destory(p);
	return -1;
}

r2v_rdp_t *
r2v_rdp_init(int client_fd)
{
	r2v_rdp_t *r = NULL;

	r = (r2v_rdp_t *)malloc(sizeof(r2v_rdp_t));
	if (r == NULL) {
		goto fail;
	}
	memset(r, 0, sizeof(r2v_rdp_t));

	r->sec = r2v_sec_init(client_fd);
	if (r->sec == NULL) {
		goto fail;
	}

	if (r2v_rdp_build_conn(r) == -1) {
		goto fail;
	}

	return r;

fail:
	r2v_rdp_destory(r);
	return NULL;
}

void
r2v_rdp_destory(r2v_rdp_t *r)
{
	if (r == NULL) {
		return;
	}
	if (r->sec != NULL) {
		r2v_sec_destory(r->sec);
	}
	free(r);
}
