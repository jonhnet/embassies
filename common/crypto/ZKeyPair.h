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

#include "ZPubKey.h"
#include "ZPrivKey.h"

class ZKeyPair {
public:
	// Generates an anonymous keypair
	ZKeyPair(RandomSupply* random_supply);

	// Generates a fresh, labeled keypair
	ZKeyPair(RandomSupply* random_supply, const DomainName* owner);
	
	// Create a keypair from one we previously serialized
	ZKeyPair(uint8_t* serializedKeyPair, uint32_t size);

	~ZKeyPair();

	// Bytes needed to serialize this keypair
	uint32_t size();
	uint32_t serialize(uint8_t* buffer);

	ZPubKey* getPubKey() { return pub; }
	ZPrivKey* getPrivKey() { return priv; }

private:
	ZPubKey* pub;
	ZPrivKey* priv;

	rsa_context* rsa_ctx;
};
