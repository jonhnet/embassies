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
#include "ZPubKey.h"

class ZPrivKey : public ZKey {
public:
	// Create a key from one we previously serialized
	ZPrivKey(uint8_t* serializedKey, uint32_t size);

	// Create a key from an existing context
	ZPrivKey(rsa_context* rsa_ctx, const DomainName* owner, ZPubKey* pubKey);

	~ZPrivKey();

	uint16_t getTag() { return pubKey->getTag(); }

	ZPubKey* getPubKey() { return pubKey; }

	// Bytes needed to serialize this key
	uint32_t size();
	uint32_t serialize(uint8_t* buffer);
	
	// Assumes signature is at least ZKey::SIGNATURE_SIZE bytes
	void sign(uint8_t* buffer, uint32_t length, uint8_t* signature);

private:
	ZPubKey* pubKey;		// Keep track of the public key associated with this one
};