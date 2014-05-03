#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

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
	if (s->epoll_fd != 0) {
		close(s->epoll_fd);
	}
	free(s);
}

void
r2v_session_transmit(r2v_session_t *s)
{
	int i, nfds;
	struct epoll_event ev, events[MAX_EVENTS];

	s->epoll_fd = epoll_create(2);
	if (s->epoll_fd == -1) {
		goto fail;
	}

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = s->vnc->fd;

	if (epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, s->vnc->fd, &ev) == -1) {
		goto fail;
	}

	while (1) {
		nfds = epoll_wait(s->epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			goto fail;
		}
		for (i = 0; i < nfds; i++) {
			if (events[i].data.fd == s->vnc->fd) {
				r2v_vnc_process_data(s->vnc);
				goto fail;
			}
		}
	}

fail:
	return;
}
