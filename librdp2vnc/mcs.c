#include <stdlib.h>
#include <string.h>

#include "mcs.h"

r2v_mcs_t *
r2v_mcs_init(int client_fd)
{
	r2v_mcs_t *m = NULL;

	m = (r2v_mcs_t *)malloc(sizeof(r2v_mcs_t));
	if (m == NULL) {
		goto fail;
	}
	memset(m, 0, sizeof(r2v_mcs_t));

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
