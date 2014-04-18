#include <stdlib.h>
#include <string.h>

#include "rdp_conn.h"

r2v_rdp_conn_t *
r2v_rdp_conn_init(int client_fd)
{
	r2v_rdp_conn_t *c = NULL;

	c = (r2v_rdp_conn_t *)malloc(sizeof(r2v_rdp_conn_t));
	if (c == NULL) {
		goto fail;
	}
	memset(c, 0, sizeof(r2v_rdp_conn_t));

	return c;

fail:
	if (c != NULL) {
		free(c);
	}
	return NULL;
}
