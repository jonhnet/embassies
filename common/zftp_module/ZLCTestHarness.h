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

#include "ZLookupClient.h"
#include "reference_file.h"

class ZLCTestHarness {
public:
	ZLCTestHarness(ZCache *zcache, ZLookupClient *zlookup_client, int max_selector, ZLCEmit *ze);

private:
	hash_t _discover_requested_hash(ReferenceFile *rf);
	void _run_inner();
	void transfer_one(int pattern_selector);

	ZCache *zcache;
	ZLCEmit *ze;
	ZLookupClient *zlookup_client;
	int max_selector;
};
