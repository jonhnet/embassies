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
#include "ZFileRequest.h"
#include "ZFileClient.h"
#include "ZCache.h"
#include "ThreadFactory.h"
#include "PerfMeasureIfc.h"
#include "ValidatedZCB.h"

class ZFileServer {
public:
	ZFileServer(ZCache *zcache, UDPEndpoint *listen_zftp, SocketFactory *sf, ThreadFactory *tf, PerfMeasureIfc* pmi);

	// cache pokes here after being fed a request by run()
	void send_reply(ZFileRequest *req, uint32_t payload_len);

private:
	ZFileRequest *_receive();
	static void _run_trampoline(void *v_this);
	void run_worker();
	bool zero_copy_reply_optimization(ZFileRequest *req);
	bool req_is_cachable(ZFileRequest *req);
	void cache_or_discard_result(ZFileRequest *req, uint32_t payload_len);
	void unprincipled_sleep_to_emulate_asynchrony();

	ZCache *zcache;
	AbstractSocket *socket;
	
	bool use_zero_copy_reply_optimization;
	ZFileRequest* cached_req;
	ValidatedZCB* cached_vbuf;
	uint32_t cached_payload_len;
	PerfMeasureIfc* pmi;
};
