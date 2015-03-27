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

#define DEBUG_COMPRESSION 0
#if DEBUG_COMPRESSION
class DebugCompression {
public:
	virtual void start_data(uint32_t start, uint32_t size) = 0;
	virtual void end_data() = 0;
};
#endif // DEBUG_COMPRESSION

class Buf {
protected:
	Buf();
	uint32_t header_offset;
	virtual uint8_t* get_data_buf() = 0;
		// used by this superclass to allocate header blocks

public:
	virtual ~Buf() {}
	virtual uint8_t* write_header(uint32_t len);
		// allocate fixed len bytes at "end" of packet and return pointer to them.
		// never compressed.
	virtual void write_data(ZBlockCacheRecord* block, uint32_t data_offset, uint32_t block_offset, uint32_t len) = 0;
		// load data from block, possibly including compression.
		// Put it at data_offset bytes after end of header portion
		// (in uncompressed address space)
	virtual uint32_t get_payload_len() = 0;
		// return amount of data actually loaded, in compressed address space.
	uint32_t get_packet_len();
	virtual CompressionContext get_compression_context();
	virtual void recycle();

//	virtual uint8_t* data() = 0;

	virtual void resetDebugStats() {}
	virtual void printDebugStats() {}
#if DEBUG_COMPRESSION
	virtual DebugCompression* debug_compression() { return NULL; }
#endif // DEBUG_COMPRESSION
};
