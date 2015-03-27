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

/*
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
*/

#include "zftp_protocol.h"
#include "malloc_factory.h"
#include "InternalFileResult.h"
#include "ZLCEmit.h"

#define LENGTH_LIMIT 1*32768
	// how long non-synthetic (test-mode) requests may be.

class ZFileRequest {
public:
	ZFileRequest(
		MallocFactory *mf,
		uint8_t *bytes,
		uint32_t len);
	ZFileRequest(MallocFactory *mf);

	virtual ~ZFileRequest();
	
	bool parse(ZLCEmit *ze, bool request_is_synthetic);

	virtual void deliver_result(InternalFileResult *result) = 0;

	hash_t *get_hash() { return &zrp->file_hash; }

	uint32_t get_padding_request() { return padding_request; }
	CompressionContext get_compression_context() { return compression_context; }
	DataRange get_data_range() { return data_range; }

	uint32_t get_num_tree_locations();
	uint32_t get_tree_location(uint32_t idx);

protected:
	void load(uint8_t *bytes, uint32_t len);

private:
	uint32_t num_tree_locations;
	uint32_t *tree_locations;	// ptr to array of network-ordered locations

public:
	bool request_is_synthetic;
	uint8_t *bytes;
	uint32_t len;

	// populated by parse()
	ZFTPRequestPacket *zrp;
	char *url_hint;

	bool is_semantically_equivalent(ZFileRequest *other);

private:
	MallocFactory *mf;
	DataRange data_range;
	uint32_t padding_request;
	CompressionContext compression_context;
};

