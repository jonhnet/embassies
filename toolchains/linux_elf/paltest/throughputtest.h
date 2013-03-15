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

#include "pal_abi/pal_abi.h"
#include "xax_skinny_network.h"

class ThroughputTest {
private:
	ZoogDispatchTable_v1 *zdt;
	XaxSkinnyNetwork *xsn;
	XaxSkinnySocket skinny_socket;
	ZoogNetBuffer *request_znb;
	char buf[1024];
	uint32_t received_total;
	debug_logfile_append_f *debuglog;
	XIPAddr *server;

	void setup();
	void ping_throughput_server();
	void msg(const char *m);

public:
	ThroughputTest(
		ZoogDispatchTable_v1 *zdt,
		XaxSkinnyNetwork *xsn,
		XIPAddr *server);
	void run();
};

