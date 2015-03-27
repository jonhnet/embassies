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
#if ZLC_USE_ZLIB
#include <zlib.h>
#endif // ZLC_USE_ZLIB

#include "zftp_protocol.h"
#include "zftp_hash_lookup_protocol.h"
#include "xax_network_utils.h"
#include "args_lib.h"
#include "ZLCArgs.h"
#include "ZFetch.h"

void ZLCArgs::_init_empty()
{
	index_dir = NULL;

	origin_lookup = NULL;
	origin_zftp = NULL;
	origin_filesystem = false;
	origin_reference = false;

	listen_lookup = NULL;
	listen_zftp = NULL;
	use_tcp = false;
	max_payload = 65400;

	run_test_harness = 0;
	warm_url = NULL;
	fetch_url = NULL;
	show_payload = true;
	probe = false;
	dump_db = false;
	print_net_stats = false;
	log_paths = NULL;
	fetch_timeout_ms = ZFetch::DEFAULT_TIMEOUT;

	compression_args.use_compression = false;
	compression_args.zlib_compression_level = 9;
	compression_args.zlib_compression_strategy = 0;
#if ZLC_USE_ZLIB
	compression_args.zlib_compression_strategy = Z_DEFAULT_STRATEGY;
#endif // ZLC_USE_ZLIB
}

ZLCArgs::ZLCArgs()
{
	_init_empty();
}

#if ZLC_USE_PRINTF

void decode_compression_strategy(int* dest, const char* value)
{
#if ZLC_USE_ZLIB
	if (lite_strcmp(value, "Z_RLE")==0)
	{
		*dest = Z_RLE;
	} else if (lite_strcmp(value, "Z_DEFAULT_STRATEGY")==0) {
		*dest = Z_DEFAULT_STRATEGY;
	} else {
		fprintf(stderr, "Unknown compression strategy %s\n", value);
		lite_assert(false);
	}
#else
	fprintf(stderr, "zlib not supported in this build\n");
	lite_assert(false);
#endif // ZLC_USE_ZLIB
}

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
		CONSUME_BOOL("--use-tcp", args->use_tcp);
		CONSUME_INT("--max-payload", args->max_payload);
		CONSUME_INT("--run-test-harness", args->run_test_harness);
		CONSUME_STRING("--index-dir", args->index_dir);
		CONSUME_STRING("--warm-url", args->warm_url);
		CONSUME_STRING("--fetch-url", args->fetch_url);
		CONSUME_OPTION_ASSIGN("--no-show-payload", show_payload, true, false);
		CONSUME_BOOL("--probe", args->probe);
		CONSUME_BOOL("--dump-db", args->dump_db);
		CONSUME_BOOL("--print-net-stats", args->print_net_stats);
		CONSUME_STRING("--log-paths", args->log_paths);
		CONSUME_OPTION_ASSIGN("--no-timeout", fetch_timeout_ms, 1500, -1);
		CONSUME_BOOL("--use-compression", args->compression_args.use_compression);
		CONSUME_INT("--zlib-compression-level", args->compression_args.zlib_compression_level);
		CONSUME_BYFUNC("--zlib-compression-strategy", decode_compression_strategy, &args->compression_args.zlib_compression_strategy);
		char buf[500]; snprintf(buf, sizeof(buf), "Unknown argument: %s", argv[0]);
		check(false, buf);
	}
}
#endif // ZLC_USE_PRINTF

