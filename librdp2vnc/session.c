#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "log.h"
#include "rdp.h"
#include "session.h"
#include "vnc.h"

r2v_session_t *
r2v_session_init(int client_fd, int server_fd, const char *password)
{
	r2v_session_t *s = NULL;

	s = (r2v_session_t *)malloc(sizeof(r2v_session_t));
	if (s == NULL) {
		goto fail;
	}
	memset(s, 0, sizeof(r2v_session_t));

	/* connect to VNC server */
	s->vnc = r2v_vnc_init(server_fd, password, s);
	if (s->vnc == NULL) {
		r2v_log_error("connect to vnc server error");
		goto fail;
	}
	r2v_log_info("connect to vnc server success");

	/* accept RDP connection */
	s->rdp = r2v_rdp_init(client_fd, s);
	if (s->rdp == NULL) {
		r2v_log_error("accept new rdp connection error");
		goto fail;
	}
	r2v_log_info("accept new rdp connection success");

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
	int i, nfds, rdp_fd, vnc_fd;
	struct epoll_event ev, events[MAX_EVENTS];

	s->epoll_fd = epoll_create(2);
	if (s->epoll_fd == -1) {
		goto fail;
	}

	rdp_fd = s->rdp->sec->mcs->x224->tpkt->fd;
	vnc_fd = s->vnc->fd;

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = rdp_fd;
	if (epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, rdp_fd, &ev) == -1) {
		goto fail;
	}

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = vnc_fd;
	if (epoll_ctl(s->epoll_fd, EPOLL_CTL_ADD, vnc_fd, &ev) == -1) {
		goto fail;
	}

	r2v_log_info("session transmit start");

	while (1) {
		nfds = epoll_wait(s->epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			goto fail;
		}
		for (i = 0; i < nfds; i++) {
			if (events[i].data.fd == rdp_fd) {
				if (r2v_rdp_process(s->rdp) == -1) {
					goto fail;
				}
			} else if (events[i].data.fd == vnc_fd) {
				if (r2v_vnc_process(s->vnc) == -1) {
					goto fail;
				}
			}
		}
	}

fail:
	r2v_log_info("session transmit end");
	return;
}
