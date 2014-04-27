#include <stdlib.h>
#include <string.h>

#include "rdp.h"
#include "session.h"
#include "vnc.h"

r2v_session_t *
r2v_session_init(int client_fd)
{
	r2v_session_t *s = NULL;

	s = (r2v_session_t *)malloc(sizeof(r2v_session_t));
	if (s == NULL) {
		goto fail;
	}
	memset(s, 0, sizeof(r2v_session_t));

	return s;

fail:
	r2v_session_destory(s);
	return NULL;
}

void
r2v_session_destory(r2v_session_t *s)
{
	if (s == NULL) {
		return;
	}
	if (s->rdp != NULL) {
		r2v_rdp_destory(s->rdp);
	}
	if (s->vnc != NULL) {
		r2v_vnc_destory(s->vnc);
	}
	free(s);
}
