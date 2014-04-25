#include <stdlib.h>
#include <string.h>

#include "rdp.h"

r2v_rdp_t *
r2v_rdp_init(int client_fd)
{
	r2v_rdp_t *c = NULL;

	c = (r2v_rdp_t *)malloc(sizeof(r2v_rdp_t));
	if (c == NULL) {
		goto fail;
	}
	memset(c, 0, sizeof(r2v_rdp_t));

	c->sec = r2v_sec_init(client_fd);
	if (c->sec == NULL) {
		goto fail;
	}

	return c;

fail:
	r2v_rdp_destory(c);
	return NULL;
}

void
r2v_rdp_destory(r2v_rdp_t *c)
{
	if (c == NULL) {
		return;
	}
	if (c->sec != NULL) {
		r2v_sec_destory(c->sec);
	}
	free(c);
}
