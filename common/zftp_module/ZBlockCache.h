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
#include "hash_table.h"
#include "ZBlockCacheRecord.h"
#include "SyncFactory.h"

class ZCache;

class ZBlockCache {
public:
	ZBlockCache(ZCache *zcache, MallocFactory *mf, SyncFactory *sf);
	~ZBlockCache();

	ZBlockCacheRecord *lookup(const hash_t *hash);
	bool insert(ZBlockCacheRecord *block);
		// returns true if this is new data

private:
	static void _free_block(void *v_this, void *v_zbcr);
	ZBlockCacheRecord *_lookup_nolock(const hash_t *hash);

private:
	SyncFactoryMutex *mutex;
	HashTable ht;
	ZCache *zcache;
};
