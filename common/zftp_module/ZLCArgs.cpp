#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#include <WinSock2.h>
#define snprintf _snprintf
#else
#include <netinet/in.h>
#endif // _WIN32
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#include "zftp_protocol.h"
#include "zftp_hash_lookup_protocol.h"
#include "xax_network_utils.h"
#include "args_lib.h"
#include "ZLCArgs.h"

void ZLCArgs::_init_empty()
{
	index_dir = NULL;

	origin_lookup = NULL;
	origin_zftp = NULL;
	origin_filesystem = false;
	origin_reference = false;

	listen_lookup = NULL;
	listen_zftp = NULL;
	max_payload = 65400;

	run_test_harness = 0;
	fetch_url = NULL;
	probe = false;
	dump_db = false;
	log_paths = NULL;
}

ZLCArgs::ZLCArgs()
{
	_init_empty();
}

#if ZLC_USE_PRINTF

UDPEndpoint *parse_endpoint(char *endpoint, int default_port, int tunid)
{
	if (lite_strcmp(endpoint, "none")==0)
	{
		return NULL;
	}
	else if (lite_strcmp(endpoint, "tunid")==0)
	{
		lite_assert(tunid!=-1);
		char buf[300];
		snprintf(buf, sizeof(buf), "10.%d.0.1:%d", tunid, default_port);
		return parse_endpoint(buf, default_port, NULL);
	}
	else
	{
		int a, b, c, d, port;
		int rc = sscanf(endpoint, "%d.%d.%d.%d:%d", &a, &b, &c, &d, &port);
		if (rc!=5)
		{
			char buf[500];
			snprintf(buf, sizeof(buf), "Unparseable endpoint: '%s'", endpoint);
			check(false, buf);
		}
		UDPEndpoint *result =
			(UDPEndpoint *) malloc(sizeof(UDPEndpoint));
		XIP4Addr addr4 = literal_ipv4addr(a, b, c, d);
		result->ipaddr = ip4_to_xip(&addr4);
		result->port = htons(port);
		return result;
	}
}

ZLCArgs::ZLCArgs(int argc, char **argv, int tunid)
{
	_init_empty();

	argc-=1; argv+=1;

	ZLCArgs *args = this;
	while (argc>0)
	{
		CONSUME_ARG("--origin-lookup", args->origin_lookup, parse_endpoint(value, ZFTP_HASH_LOOKUP_PORT, tunid));
		CONSUME_ARG("--origin-zftp", args->origin_zftp, parse_endpoint(value, ZFTP_PORT, tunid));
		CONSUME_BOOL("--origin-filesystem", args->origin_filesystem);
		CONSUME_BOOL("--origin-reference", args->origin_reference);
		CONSUME_ARG("--listen-lookup", args->listen_lookup, parse_endpoint(value, ZFTP_HASH_LOOKUP_PORT, tunid));
		CONSUME_ARG("--listen-zftp", args->listen_zftp, parse_endpoint(value, ZFTP_PORT, tunid));
		CONSUME_INT("--max-payload", args->max_payload);
		CONSUME_INT("--run-test-harness", args->run_test_harness);
		CONSUME_BOOL("--dump-db", args->dump_db);
		CONSUME_STRING("--index-dir", args->index_dir);
		CONSUME_STRING("--fetch-url", args->fetch_url);
		CONSUME_BOOL("--probe", args->probe);
		CONSUME_STRING("--log-paths", args->log_paths);
		char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]);
		check(false, buf);
	}
}
#endif // ZLC_USE_PRINTF
