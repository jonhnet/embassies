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

#include "ZKey.h"
#include "ZCert.h"
#include "ZPubKeyRecord.h"
#include "ZCertChain.h"

// Forward declare these, since they're mutually recursive
class ZPubKeyRecord;
class ZCert;
class ZCertChain;

class ZPubKey : public ZKey {
public:	
	// Duplicate an existing key
	ZPubKey(ZPubKey* key);

	// Create a new key from the key bytes in a DNSKEY RR
	ZPubKey(uint16_t algorithm, uint8_t* keyBytes, uint32_t keyBytesSize, DomainName* owner);

	// Create a key from one we previously serialized
	ZPubKey(uint8_t* serializedKey, uint32_t size);	

	// Create a key from an existing context
	ZPubKey(rsa_context* rsa_ctx, const DomainName* owner);

	~ZPubKey();

	ZPubKeyRecord* getRecord() { return this->record; }
	uint16_t getTag();

	// Bytes needed to serialize this key
	uint32_t size();
	// Assumes buffer is at least size() bytes
	uint32_t serialize(uint8_t* buffer);

	rsa_context* getContext() { return this->rsa_ctx; }

	// Verify that signature is valid for the contents in buffer
	// Assumes signature is at least ZKey::SIGNATURE_SIZE bytes
	bool verify(uint8_t* signature, uint8_t* buffer, uint32_t length);

	// Verify the validity of the label on this key using the cert chain provided, with trustAnchor as a root of trust
	// At each step, the label on the signed object must have the parent's label chain as a suffix;
	// i.e., .com can sign example.com, which can sign www.example.com, but neither can sign harvard.edu
	// Chain should be arranged in order of increasing specificity;
	// i.e., trustAnchor signs key1 signs key2 signs key3 ... signs this key
	bool verifyLabel(ZCertChain* chain, ZPubKey* trustAnchor);

	bool isEqual(ZPubKey* key);

private:	
	ZPubKeyRecord* record;
	
	// Return number of bytes needed for this key's actual key material
	uint32_t keyRecordSize();

	// Return actual key bytes for use in a pubkey record
	// Assumes buffer is at least keyRecordSize() bytes
	void getKeyRecordBytes(uint8_t* buffer);
};