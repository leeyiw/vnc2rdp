#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "vnc.h"

static int
r2v_vnc_build_conn(r2v_vnc_t *v)
{
	v->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (v->fd == -1) {
		goto fail;
	}

	return 0;

fail:
	return -1;
}

r2v_vnc_t *
r2v_vnc_init()
{
	r2v_vnc_t *v = NULL;

	v = (r2v_vnc_t *)malloc(sizeof(r2v_vnc_t));
	if (v == NULL) {
		goto fail;
	}
	memset(v, 0, sizeof(r2v_vnc_t));

	if (r2v_vnc_build_conn(v) == -1) {
		goto fail;
	}

	return v;

fail:
	r2v_vnc_destory(v);
	return NULL;
}

void
r2v_vnc_destory(r2v_vnc_t *v)
{
	if (v == NULL) {
		return;
	}
	if (v->fd != 0) {
		close(v->fd);
	}
	free(v);
}
