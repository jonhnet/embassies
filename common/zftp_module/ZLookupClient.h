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

/*
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
*/

#include "SocketFactory.h"
#include "SyncFactory.h"

class ZLookupClient {
private:
	UDPEndpoint *origin_lookup;	// where to send requests
	SocketFactory *sockf;
	uint32_t nonce;
	SyncFactoryMutex *mutex;	// serialize requests to make nonce rendezvous trivial
	ZLCEmit *ze;

	AbstractSocket *sock;

	void _open_socket();

public:
	ZLookupClient(UDPEndpoint *origin_lookup, SocketFactory *sockf, SyncFactory *syncf, ZLCEmit *ze);
	hash_t lookup_url(const char *url);
};
