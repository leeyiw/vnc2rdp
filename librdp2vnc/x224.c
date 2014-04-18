#include <stdlib.h>
#include <string.h>

#include "x224.h"

r2v_x224_t *
r2v_x224_init(int client_fd)
{
	r2v_x224_t *x = NULL;

	x = (r2v_x224_t *)malloc(sizeof(r2v_x224_t));
	if (x == NULL) {
		goto fail;
	}
	memset(x, 0, sizeof(r2v_x224_t));

	return x;

fail:
	r2v_x224_destory(x);
	return NULL;
}

void
r2v_x224_destory(r2v_x224_t *x)
{
	if (x == NULL) {
		return;
	}
	free(x);
}
