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

#include "ZCache.h"
#include "ZLookupClient.h"
#include "ValidFileResult.h"

class ZSyntheticFileRequest;

class ZFetch {
public:
	ZFetch(ZCache *zcache, ZLookupClient *zlookup_client);
	~ZFetch();
	bool fetch(const char *url);

	uint32_t get_payload_size() { return payload_size; }
	uint8_t *get_payload();
		// caller must return result to free_payload()
	void free_payload(uint8_t *payload);

	void dbg_display();

private:
	uint8_t *_read(uint32_t *offset, uint32_t len);

	ZCache *zcache;
	ZLookupClient *zlookup_client;
	ZSyntheticFileRequest *zreq;

	ValidFileResult *result;	// copy of data ptr owned by zreq; we don't delete.
	uint32_t payload_size;
	MallocFactory *mf;
};
