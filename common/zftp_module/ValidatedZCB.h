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

#include "pal_abi/pal_types.h"
#include "zftp_protocol.h"
#include "ZBlockCacheRecord.h"
#include "ZeroCopyBuf.h"
#include "SocketFactory.h"
#include "Buf.h"
#include "ZCache.h"

class ValidatedZCB : public Buf  {
public:
	// Buffer we represent, and the socket it came from
	ValidatedZCB(ZCache* cache, ZeroCopyBuf* buffer, AbstractSocket* socket);
	~ValidatedZCB();
	void read(ZBlockCacheRecord* block, uint32_t buf_offset, uint32_t block_offset, uint32_t len);
	uint8_t* data() { return buffer->data(); }

	ZeroCopyBuf* get_zcb() { return buffer; }

	void set_debug_offset(uint32_t offset) { debug_offset = offset;}

	ZCache* cache;

	void resetDebugStats();
	void printDebugStats();

private:
	ZeroCopyBuf* buffer;
	AbstractSocket* socket;
	hash_t* hashes;
	uint32_t debug_offset;

	uint32_t cache_hits;
	uint32_t cache_misses;
};
