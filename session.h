#ifndef _SESSION_H_
#define _SESSION_H_

#define MAX_EVENTS					64

typedef struct _r2v_rdp_t r2v_rdp_t;
typedef struct _r2v_vnc_t r2v_vnc_t;

typedef struct _r2v_session_t {
	r2v_rdp_t *rdp;
	r2v_vnc_t *vnc;
	int epoll_fd;
} r2v_session_t;

extern r2v_session_t *r2v_session_init(int client_fd, int server_fd,
									   const char *password);
extern void r2v_session_destory(r2v_session_t *s);
extern void r2v_session_transmit(r2v_session_t *s);

#endif
