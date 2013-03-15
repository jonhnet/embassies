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

#include "linked_list.h"
#include "ZNameRecord.h"
#include "ZLookupRequest.h"
#include "ZCachedStatus.h"
#include "SyncFactory.h"

class ZCache;

class ZCachedName {
public:
	ZCachedName(ZCache *zcache, const char *url_hint, MallocFactory *mf);
	~ZCachedName();

	void consider_request(ZLookupRequest *req);

	void install_from_origin(ZNameRecord *znr);

	static uint32_t __hash__(const void *a);
	static int __cmp__(const void *a, const void *b);

	hash_t *get_file_hash();
		// asserts file is already resolved.
	hash_t *known_validated_hash();
		// returns non-NULL only if hash is resolved *and* mtime valid.

private:
	void _consider_request_nolock(ZLookupRequest *req);
	void _signal_waiters();
	void _update_status(ZCachedStatus new_status);
	void _install(ZNameRecord *znr, ZCachedStatus new_status);
	void _check_freshness();

	ZCache *zcache;
	char *url_hint;
	ZNameRecord *znr;

	SyncFactoryMutex *mutex;
	MallocFactory *mf;
	LinkedList waiters;

	ZCachedStatus status;
};
