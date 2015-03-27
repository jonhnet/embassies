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

#include "avl2.h"
#include "ValidatedMerkleRecord.h"
#include "ZCachedFile.h"
#include "OutboundRequest.h"

typedef AVLTree<Avl_uint32_t, uint32_t> LocationAVLTree;

class RequestBuilder {
private:
	ZCachedFile *zcf;
	uint32_t mtu;
	const char *url_hint;
		// tree contains pointers into objects (ValidatedMerkleRecords)
		// whose lifetimes are managed beyond this object,
		// so we don't delete them when we destruct the tree.

	LocationAVLTree committed_tree_locations;
	LocationAVLTree tentative_tree_locations;

	DataRange request_range;

	void request_block_hash(uint32_t block_num);
	bool request_hash(uint32_t location);
	void request_hash_ancestors(uint32_t block_num);

	int _available();
	void _build_request(DataRange missing_set);

	enum Disposition { COMMIT, ABORT };
	void clear_tentative_locations(Disposition disposition);
	void optimize_requested_hash_list();

public:
	RequestBuilder(ZCachedFile *zcf, SyncFactory *sf, DataRange missing_set, uint32_t mtu);
	~RequestBuilder();
	OutboundRequest *as_outbound_request(CompressionContext compression_context);

#if 0 	// dead code?
	static ZFTPRequestPacket *make_packet(
		MallocFactory *mf,
		const hash_t *file_hash,
		const char *url_hint,
		uint32_t num_tree_locations,
		uint32_t **out_tree_locations,
		uint32_t padding_request,
		uint32_t data_start,
		uint32_t data_end);
#endif

	bool requested_any_data();
};
