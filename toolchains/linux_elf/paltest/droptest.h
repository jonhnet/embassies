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

#include "xax_skinny_network.h"
#include "SyncFactory_Zutex.h"
#include "ZCache.h"
#include "ZLCArgs.h"
#include "SocketFactory.h"
#include "ZLCEmitXdt.h"
#include "ZLookupClient.h"

class DropTest {
private:
	ZoogDispatchTable_v1 *zdt;
	MallocFactory *mf;
	XaxSkinnyNetwork *xsn;
	SyncFactory *sf;

	ZLCEmit *ze;

	ZLCArgs zlcargs;
	ZCache *zcache;
	SocketFactory *socket_factory;
	ZFileClient *zfile_client;
	ZLookupClient *zlookup_client;

	void _setup_server_addrs(
		XIPAddr *server,
		UDPEndpoint *out_lookup_ep,
		UDPEndpoint *out_zftp_ep);

public:
	DropTest(
		ZoogDispatchTable_v1 *zdt,
		MallocFactory *mf,
		XaxSkinnyNetwork *xsn,
		XIPAddr *server);
	~DropTest();
	void run();
};

