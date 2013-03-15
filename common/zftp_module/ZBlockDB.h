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

#include "ZBlockCacheRecord.h"
#include "zftp_protocol.h"

#include "ZDB.h"

class ZBlockDB {
public:
	ZBlockDB(ZFTPIndexConfig *zic, const char *index_dir, MallocFactory *mf);
	~ZBlockDB();

	void insert(ZBlockCacheRecord *block);
	ZBlockCacheRecord *lookup(const hash_t *block_hash);

	void debug_dump();

private:
	ZDB *zdb;
	MallocFactory *mf;
};
