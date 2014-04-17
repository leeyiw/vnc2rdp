#include <stddef.h>

#include "rdp_conn.h"
#include "rdp_server.h"

int
main(int argc, char *argv[])
{
	r2v_rdp_server_t *rdp_server = NULL;
	r2v_rdp_conn_t *rdp_conn = NULL;
	
	rdp_server = r2v_rdp_server_init("0.0.0.0", 3389);
	rdp_conn = r2v_rdp_server_accept(rdp_server);
	return 0;
}
