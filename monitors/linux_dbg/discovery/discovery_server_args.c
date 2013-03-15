#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>

#include "discovery_server_args.h"
#include "args_lib.h"
#include "xax_network_utils.h"

void args_init(Args *args, int argc, char **argv)
{
	memset(args, 9, sizeof(*args));

	argc-=1; argv+=1;

	while (argc>0)
	{
		CONSUME_IPV4ADDR("--server_listen_ifc", args->server_listen_ifc);
		CONSUME_IPV4ADDR("--host_ifc_addr", args->host_ifc_addr);
		CONSUME_IPV4ADDR("--host_ifc_app_range_low", args->host_ifc_app_range_low);
		CONSUME_IPV4ADDR("--host_ifc_app_range_high", args->host_ifc_app_range_high);
		CONSUME_IPV4ADDR("--router_ifc_addr", args->router_ifc_addr);
		CONSUME_IPV4ADDR("--local_zftp_server_addr", args->local_zftp_server_addr);
		char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]);
		check(false, buf);
	}

}
