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

#include "malloc_factory.h"
#include "zftp_protocol.h"
#include "DataRange.h"

class OutboundRequest {
private:
	MallocFactory *mf;
	ZFTPRequestPacket *zrp;
	uint32_t len;
	uint32_t *locations;
	uint32_t dbg_locations_remaining;

public:
	OutboundRequest(
		MallocFactory *mf,
		const hash_t *file_hash,
		const char *url_hint,
		uint32_t num_tree_locations,
		uint32_t padding_request,
		DataRange data_range);

	~OutboundRequest();

	void set_location(uint32_t i, uint32_t location);
	
	uint32_t get_len() { return len; }
	ZFTPRequestPacket *get_zrp();
};
