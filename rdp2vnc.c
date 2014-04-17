#include <stddef.h>

#include "rdp_server.h"

int
main(int argc, char *argv[])
{
	r2v_rdp_server_t *rdp_server = NULL;
	rdp_server = r2v_rdp_server_init("0.0.0.0", 3389);
	return 0;
}
