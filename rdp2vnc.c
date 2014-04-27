#include <stddef.h>
#include <stdlib.h>

#include "log.h"
#include "rdp_server.h"
#include "session.h"
#include "vnc.h"

int
main(int argc, char *argv[])
{
	r2v_rdp_server_t *rdp_server = NULL;
	r2v_session_t *session = NULL;
	
	rdp_server = r2v_rdp_server_init("0.0.0.0", 3389);
	if (rdp_server == NULL) {
		r2v_log_error("init rdp server error");
		exit(EXIT_FAILURE);
	}
	r2v_log_info("init rdp server success");
	while (1) {
		session = r2v_session_init();
		session->rdp = r2v_rdp_server_accept(rdp_server);
		if (session->rdp == NULL) {
			r2v_log_error("accept new rdp connection error");
			r2v_session_destory(session);
			continue;
		}
		r2v_log_info("accept new rdp connection success");
		session->vnc = r2v_vnc_init();
		if (session->vnc == NULL) {
			r2v_log_error("connect to vnc server error");
			r2v_session_destory(session);
			continue;
		}
		r2v_log_info("connect to vnc server success");
		r2v_session_destory(session);
	}
	return 0;
}
