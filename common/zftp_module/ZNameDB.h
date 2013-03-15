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

#include "zftp_protocol.h"
#include "FreshnessSeal.h"
#include "ZFTPIndexConfig.h"
#include "ZDB.h"
#include "ZNameRecord.h"

class ZNameDB {
public:
	ZNameDB(ZCache *zcache, ZFTPIndexConfig *zic, const char *index_dir);
	~ZNameDB();
	void insert(const char *url, ZNameRecord *znr);
		// overwrites a previous entry
	ZNameRecord *lookup(const char *url);

	void debug_dump();

private:
	ZCache *zcache;	// needed for decoding ZOrigin classes from marshalled names.
	ZDB *zdb;
};
