#include <stddef.h>
#include <stdlib.h>

#include "rdp.h"
#include "rdp_server.h"
#include "log.h"

int
main(int argc, char *argv[])
{
	r2v_rdp_server_t *rdp_server = NULL;
	r2v_rdp_t *rdp = NULL;
	
	rdp_server = r2v_rdp_server_init("0.0.0.0", 3389);
	if (rdp_server == NULL) {
		r2v_log_error("init rdp server error");
		exit(EXIT_FAILURE);
	}
	r2v_log_info("init rdp server success");
	while (1) {
		rdp = r2v_rdp_server_accept(rdp_server);
		if (rdp == NULL) {
			r2v_log_error("accept new rdp connection error");
			continue;
		}
		r2v_log_info("accept new rdp connection success");
		r2v_rdp_destory(rdp);
	}
	return 0;
}
