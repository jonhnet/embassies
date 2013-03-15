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

#include "ZFileRequest.h"
#include "SyncFactory.h"

// A request class designed for programmatic use from inside an app (e.g.
// in the elf_loader boot block): it's initialized by hash
// (& optional offset, len parameters),
// and returns results via a blocking wait_reply() call.

class ZSyntheticFileRequest : public ZFileRequest {
public:
	ZSyntheticFileRequest(ZLCEmit *ze, MallocFactory *mf, hash_t *hash, SyncFactory *sf);
	ZSyntheticFileRequest(ZLCEmit *ze, MallocFactory *mf, hash_t *hash, SyncFactory *sf, uint32_t data_start, uint32_t data_end);
	~ZSyntheticFileRequest();

	void deliver_result(InternalFileResult *result);
	InternalFileResult *wait_reply(int timeout_ms);
		// result belongs to this and will be deleted with this. Caller should
		// not delete.

protected:
	SyncFactoryEvent *event;
	InternalFileResult *result;

private:
	void _init(ZLCEmit *ze, hash_t *hash, SyncFactory *sf, uint32_t data_start, uint32_t data_end);
};


