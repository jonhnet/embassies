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
#include "ZDBDatum.h"

class ZCache;
class ZOrigin;

class ZNameRecord {
public:
	ZNameRecord(ZOrigin *origin, hash_t *file_hash, FreshnessSeal *seal);
	ZNameRecord(ZNameRecord *znr);
	~ZNameRecord();

	ZNameRecord(ZCache *zcache, ZDBDatum *d);
	ZDBDatum *marshal();

	inline hash_t *get_file_hash() { return &file_hash; }
	inline FreshnessSeal *get_seal() { return &seal; }

	bool check_freshness(const char *url);

private:
	void _marshal(ZDBDatum *zd);

	ZOrigin *origin;
	hash_t file_hash;
	FreshnessSeal seal;
};
