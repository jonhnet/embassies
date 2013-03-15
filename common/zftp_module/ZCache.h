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

#include "hash_table.h"
#include "ZFileRequest.h"
#include "ZCachedFile.h"
#include "ZCachedName.h"
#include "ZLookupRequest.h"
#include "ZBlockCache.h"
#include "ZNameDB.h"
#include "ZFileDB.h"
#include "ZBlockDB.h"
#include "ZOrigin.h"
#include "ThreadFactory.h"

class ZLookupRequest;
class ZFileServer;
class ZFileClient;
class ZFSOrigin;
class ZLCArgs;

class ZCache {
public:
	ZCache(ZLCArgs *zargs, MallocFactory *mf, SyncFactory *sf, ZLCEmit *ze, SendBufferFactory *sbf);
	~ZCache();

	void configure(
		ZFileServer *zfile_server,
		ZFileClient *zfile_client);

	ZCachedFile *lookup_hash(const hash_t *file_hash);
		// Create if absent

	ZCachedName *lookup_url(const char *url_hint);
		// Create if absent

	void probe_origins(const char *url);
	bool probe_origins_for_hash(const char *url, hash_t *hash);

	bool has_origins() { return origins.count>0; }

	ZOrigin *lookup_origin(const char *class_name);

	ZLCEmit *GetZLCEmit() { return ze; }

	void wait_until_configured() { return configured->wait(); }

	SendBufferFactory *get_send_buffer_factory() { return sbf; }

public:
	MallocFactory *mf;
	SyncFactory *sf;
	hash_t hash_of_ZFTP_BLOCK_SIZE_zeros;
	ZFileServer *zfile_server;
	ZFileClient *zfile_client;

	hash_t zero_hash;

	// In-core storage
	HashTable file_ht;
	ZBlockCache *block_cache;
	HashTable name_ht;

	bool insert_block(ZBlockCacheRecord *block);

	// origin input
	void insert_name(const char *url, ZNameRecord *znr);

	void debug_dump_db();

	// Persistent storage.
	ZFTPIndexConfig *zic;

	void store_file(ZCachedFile *zcf);
	ZCachedFile *retrieve_file(hash_t *file_hash);

	void store_name(const char *url, ZNameRecord *znr);
	ZNameRecord *retrieve_name(const char *url);

	void store_block(ZBlockCacheRecord *zbr);
	ZBlockCacheRecord *retrieve_block(const hash_t *hash);

private:
	void _add_origin(ZOrigin *origin);
	static void _delete_cached_file(void *v_this, void *datum);
	static void _delete_name_record(void *v_this, void *datum);

private:
	ZNameDB *znameDB;
	ZFileDB *zfileDB;
	ZBlockDB *zblockDB;

	LinkedList origins;	// <ZOrigin>
	HashTable name_to_origin;
	ZLCEmit *ze;
	SendBufferFactory *sbf;

	SyncFactoryEvent *configured;
};


