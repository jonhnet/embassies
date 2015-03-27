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

#include "SendBufferFactory.h"
#include "SocketFactory_Counter.h"
#include "SocketFactory.h"
#include "standard_malloc_factory.h"
#include "SyncFactory.h"
#include "ThreadFactory.h"
#include "ZCache.h"
#include "ZLCArgs.h"
#include "ZLookupClient.h"

class ZFTPApp {
private:
	ZLCArgs *zlc_args;
	SocketFactory_Counter *socket_factory_counter;
	ZLookupClient *zlookup_client;
	MallocFactory *mf;
	SyncFactory *sf;
	ThreadFactory *tf;
	ZCompressionIfc* zcompression;
	ZLCEmit *ze;
	SendBufferFactory *sbf;

	ZLookupClient* get_zlookup_client();
	void fetch(const char* fetch_url);

public:
	ZFTPApp(ZLCArgs *zlc_args, 
		  MallocFactory *mf, 
		  SocketFactory *underlying_socket_factory,
		  SyncFactory *sf,           
		  ThreadFactory *tf,
		  SendBufferFactory *sbf);
	void run();
	void close();

	ZCache *zcache;
};

