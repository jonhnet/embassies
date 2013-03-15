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
#include "pal_abi/pal_net.h"
#include "ZeroCopyBuf.h"

class ZeroCopyBuf_Xnb : public ZeroCopyBuf
{
public:
	ZeroCopyBuf_Xnb(ZoogDispatchTable_v1 *xdt, uint32_t len);
		// allocate a buffer from xdt (typically for sending)

	ZeroCopyBuf_Xnb(ZoogDispatchTable_v1 *xdt, ZoogNetBuffer *xnb, uint32_t offset, uint32_t len);
		// wrap an existing (received) buffer
		// len is bytes available *after* offset is applied.

	virtual ~ZeroCopyBuf_Xnb();
	uint8_t *data();
	uint32_t len();

//	ZoogNetBuffer *claim_xnb();
	ZoogNetBuffer *peek_xnb();
		// used for multiple calls to SocketFactory::zc_send().
		// TODO there's a latent bug here, because we have no way to know
		// when the Zoog send call is done with the packet and we can start
		// prodding the zero-copy buffer again.
	
protected:
	ZoogDispatchTable_v1 *xdt;
	ZoogNetBuffer *xnb;
	uint32_t _offset;
	uint32_t _len;
};
