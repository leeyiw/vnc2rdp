#include <stddef.h>
#include <stdlib.h>

#include "rdp_conn.h"
#include "rdp_server.h"
#include "log.h"

int
main(int argc, char *argv[])
{
	r2v_rdp_server_t *rdp_server = NULL;
	r2v_rdp_conn_t *rdp_conn = NULL;
	
	rdp_server = r2v_rdp_server_init("0.0.0.0", 3389);
	if (rdp_server == NULL) {
		r2v_log_error("init rdp server error");
		exit(EXIT_FAILURE);
	}
	r2v_log_info("init rdp server success");
	while (1) {
		rdp_conn = r2v_rdp_server_accept(rdp_server);
		if (rdp_conn == NULL) {
			r2v_log_error("accept new rdp connection error");
			exit(EXIT_FAILURE);
		}
		r2v_log_info("accept new rdp connection success");
		r2v_rdp_conn_destory(rdp_conn);
	}
	return 0;
}
