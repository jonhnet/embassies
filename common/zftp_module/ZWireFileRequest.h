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

#include "ZFileRequest.h"
#include "ZFileServer.h"
#include "unistd.h"
#include "Buf.h"

// A request class designed to service remote requests:
// initialized with the remote (reply-to) address and the raw request bytes,
// and results are delivered by handing them off to the ZFileServer's
// send_reply() mechanism.

class ZWireFileRequest : public ZFileRequest {
public:
	ZWireFileRequest(
		MallocFactory *mf,
		ZFileServer *zfileserver,
		UDPEndpoint *remote_addr,
		uint8_t *bytes,
		uint32_t len);
	virtual ~ZWireFileRequest();

	void deliver_result(InternalFileResult *result);
	void fill_payload(Buf* vbuf);
		// There's a complicated callback protocol with ZFileServer here,
		// because we're trying to keep knowledge of how zero-copy bufs are
		// constructed from spreading around. We're not doing a *great* job.
		// We need to know the payload length to learn the header length,
		// which we need to allocate the zero copy buffer.

public:
	socklen_t fromlen;
	UDPEndpoint remote_addr;
	InternalFileResult *result;

	ZFileServer *zfileserver;
};


