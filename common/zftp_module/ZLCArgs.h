/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

#include "xax_network_defs.h"
#include "linked_list.h"

typedef struct s_ZCompressionArgs {
	bool use_compression;
	int zlib_compression_level;
	int zlib_compression_strategy;
} ZCompressionArgs;

class ZLCArgs {
public:
	ZLCArgs();
	ZLCArgs(int argc, char **argv, int tunid);

	const char *index_dir;

	UDPEndpoint *origin_lookup;
	UDPEndpoint *origin_zftp;
	bool origin_filesystem;
	bool origin_reference;

	UDPEndpoint *listen_lookup;
	UDPEndpoint *listen_zftp;
	bool use_tcp;
	int max_payload;

	int run_test_harness;
	const char *warm_url;	// fetch this and then reset counters
	const char *fetch_url;
	bool show_payload;
	bool probe;
	bool dump_db;
	bool print_net_stats;
	const char *log_paths;
	int fetch_timeout_ms;
	ZCompressionArgs compression_args;

private:
	void _init_empty();
};
