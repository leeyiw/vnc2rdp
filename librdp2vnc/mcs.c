#include <stdlib.h>
#include <string.h>

#include "mcs.h"

static int
r2v_mcs_build_conn(int client_fd)
{
	return 0;
}

r2v_mcs_t *
r2v_mcs_init(int client_fd)
{
	r2v_mcs_t *m = NULL;

	m = (r2v_mcs_t *)malloc(sizeof(r2v_mcs_t));
	if (m == NULL) {
		goto fail;
	}
	memset(m, 0, sizeof(r2v_mcs_t));

	m->x224 = r2v_x224_init(client_fd);
	if (m->x224 == NULL) {
		goto fail;
	}

	if (-1 == r2v_mcs_build_conn(client_fd)) {
		goto fail;
	}

	return m;

fail:
	r2v_mcs_destory(m);
	return NULL;
}

void
r2v_mcs_destory(r2v_mcs_t *m)
{
	if (m == NULL) {
		return;
	}
	if (m->x224 != NULL) {
		r2v_x224_destory(m->x224);
	}
	free(m);
}
