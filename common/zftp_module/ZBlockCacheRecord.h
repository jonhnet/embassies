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
#include "ZDBDatum.h"
#include "malloc_factory.h"
#include "ZFileReplyFromServer.h"
#include "DataRange.h"

class ZCache;

class ZBlockCacheRecord {
private:
	MallocFactory *mf;
	hash_t hash;
	uint8_t *data;
	ZCache *zcache;	// where to store_block() myself when completely received

	uint32_t next_gap;
		// for now, assume we receive data in order.
	bool _valid;
		// all data present (next_gap==ZFTP_BLOCK_SIZE) && hash verified
		// (next_gap is only defined while !_valid.)

	void _common_init(MallocFactory *mf, const hash_t *hash);

	void _load_data_internal(uint8_t *data_source, uint32_t physical_size, uint32_t logical_size);

private:
	friend class ZBlockCache;
	ZBlockCacheRecord(const hash_t *hash);	// hash_table key ctor
	void _validate();

public:
	ZBlockCacheRecord(ZCache *zcache, const hash_t *expected_hash);
	ZBlockCacheRecord(ZCache *zcache, uint8_t *in_data, uint32_t valid_data_len);
	ZBlockCacheRecord(MallocFactory *mf, const hash_t *hash, ZDBDatum *v);
	~ZBlockCacheRecord();

//	void load_data(uint8_t *data, uint32_t valid_data_len);
	bool receive_data(
		uint32_t start,
		uint32_t end,
		ZFileReplyFromServer *reply,
		uint32_t offset);
	void complete_with_zeros();
	void get_first_gap(uint32_t *start, uint32_t *size);

	const hash_t *get_hash() { return &hash; }
	bool is_valid() { return _valid; }
	DataRange get_missing_range();
	void read(uint8_t *out_buf, uint32_t off, uint32_t len);
	const uint8_t *peek_data();

	static uint32_t __hash__(const void *a);
	static int __cmp__(const void *a, const void *b);
};


