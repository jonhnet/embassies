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

#include "ZRecord.h"
#include "ZPubKey.h"

// Forward declare these, since they're mutually recursive
class ZPubKey;
class ZPubKeyRecord;

// Derived from DNSKEY resource record
class ZPubKeyRecord : public ZRecord {
public:
	ZPubKeyRecord(uint8_t* dnskey, uint32_t size);
	ZPubKeyRecord(ZPubKeyRecord* key);
	ZPubKeyRecord(const DomainName* label, uint8_t* keyBytes, uint32_t keyBytesSize);
	~ZPubKeyRecord();

	// Write the record into the buffer and return number of bytes written
	// Assumes buffer is at least size() bytes
	uint32_t serialize(uint8_t* buffer);

	// Get a handy tag for identifying this key (not guaranteed to be unique!)
	uint16_t getTag();

	bool isEqual(const ZRecord* z) const;

	// Returns a usuable key
	ZPubKey* getKey();

private:
	// Write the just the key-related info (not the generic record info) into the buffer and return number of bytes written
	uint32_t serializeKey(uint8_t* buffer);

	uint16_t	flags;
	uint8_t		protocol;	// Always 3
	uint8_t		algorithm;	// Must be one of ZOOG_SIGN_ALGORITHM_*
	uint8_t*	key;		// Key material.  Format depends on algorithm
	ZPubKey*    pubkey;		// A key that's actually useful
};
