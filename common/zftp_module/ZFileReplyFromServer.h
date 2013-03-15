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
#include "ZeroCopyBuf.h"
#include "xax_network_utils.h"
#include "ZLCEmit.h"
#include "DataRange.h"

// This class ingests a raw reply from a ZFTP file server,
// parses and sanity-checks it,
// and provides structured access to its contents.

class ZFileReplyFromServer
{
public:
	ZFileReplyFromServer(ZeroCopyBuf *zcb, ZLCEmit *ze);
	~ZFileReplyFromServer();

	ZFTPFileMetadata *get_metadata();
	hash_t *get_filehash();

	uint32_t get_data_start() { return _data_range.start(); }
	uint32_t get_data_end() { return _data_range.end(); }

	uint32_t get_num_merkle_records();
	ZFTPMerkleRecord *get_merkle_record(uint32_t i);

	uint32_t get_merkle_tree_location(uint32_t i);
	hash_t *get_merkle_hash(uint32_t i);

#if 0	// gonna have to change
	uint8_t *access_block(uint32_t block_num, uint32_t *out_data_len);
		// block_num is in file address space, not relative to this packet's
		// first_block_num.
#endif

	uint8_t *get_payload();
	uint32_t get_payload_len();

	bool is_valid() { return _valid; }

private:
	ZeroCopyBuf *zcb;

	void assert_valid() { lite_assert(_valid); }

	bool _valid;
	ZFTPDataPacket *zdp;
	ZFTPMerkleRecord *raw_records;
	uint32_t num_merkle_records;

	uint8_t *_payload;
	uint32_t _payload_len;

	DataRange _data_range;
};
