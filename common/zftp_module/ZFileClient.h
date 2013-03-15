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

#include "zftp_protocol.h"
#include "ZCache.h"
#include "ZFileReplyFromServer.h"
#include "SocketFactory.h"
#include "ThreadFactory.h"
#include "OutboundRequest.h"

class ZFileClient {
public:
	ZFileClient(ZCache *zcache, UDPEndpoint *origin_zftp, SocketFactory *sockf, ThreadFactory *tf, uint32_t max_payload);

	// cache pokes here to make upstream request
	void issue_request(OutboundRequest *obr);

	uint32_t get_max_payload();

private:
	static void _run_trampoline(void *v_this);
	void run_worker();
	ZFileReplyFromServer *_receive();

	ZCache *zcache;
	ZLCEmit *ze;
	UDPEndpoint *origin_zftp;
	AbstractSocket *sock;
	uint32_t _max_payload;
};

