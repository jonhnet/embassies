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

#include "ZCachedFile.h"

// Origin file-hashing interface.
// Origin mode is a little weird: we're recycling parts of
// this top-down-verify class into a bottom-up-generate class.

class ZOriginFile : public ZCachedFile {
public:
	ZOriginFile(ZCache *zcache, uint32_t file_len, uint32_t flags);

	void origin_load_block(
		uint32_t block_num, uint8_t *in_data, uint32_t valid_data_len);
	void origin_finish_load();

protected:
	void _fill_in_sibling_hashes();
	void _track_parent_of_tentative_nodes(ValidatedMerkleRecord *parent);
	void _assert_root_validated();
	void _mark_node_on_block_load(uint32_t location);
};
