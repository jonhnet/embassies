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
#include "perf_measure.h"
#include "KeyVal.h"
#include "KeyValClient.h"
#include "KeyDerivationKey.h"
#include "MacKey.h"
#include "RandomSupply.h"

#define NUM_THREADS 8
#define STACK_SIZE 4096*20

typedef struct {
	Mac macs[NUM_THREADS];
	Mac metaMac;
} mac_list;
class ZSyntheticFileRequest;

class ZFastFetch {
public:
	ZFastFetch(
		ZLCEmit *ze,
		MallocFactory *mf,
		ThreadFactory *tf, 
		SyncFactory *syncf,
		ZLookupClient *zlookup_client,
		UDPEndpoint *origin_zftp,
		SocketFactory *sockf,
		PerfMeasure *perf_measure,
		KeyDerivationKey* appKey, 
		RandomSupply* random_supply
		);
	~ZFastFetch();

	// vendor_name is name of app doing the fetching
	// fetch_url is the desired file's URI
	ZeroCopyBuf *fetch(const char * vendor_name, const char *fetch_url);

private:
	ZLCEmit *ze;
	MallocFactory *mf;
	ThreadFactory *tf;
	SyncFactory *syncf;
	ZLookupClient *zlookup_client;
	UDPEndpoint *origin_zftp;
	SocketFactory *sockf;
	KeyValClient* keyval_client;
	PerfMeasure *perf_measure;
	MacKey* mac_key;
	RandomSupply* random_supply;

	// Ask key-value server for the MAC for this file
	// Expects an app and file specific string in key
	// Returns true if mac contains a purported MAC value
	bool lookup_mac(Key* key, mac_list* mac);

	// Inserts mac value into the KeyValue server's store 
	void insert_mac(Key* key, mac_list* mac);

	void distribute_hash(uint8_t* start, uint32_t num_bytes, hash_t* result);
	void distribute_mac(uint8_t* start, uint32_t num_bytes, 
                      mac_list* result, RandomSupply* random_supply,
											MacKey* key);
	bool distribute_mac_verify(uint8_t* start, uint32_t num_bytes, 
                             mac_list* old_mac_list, MacKey* key);

};
