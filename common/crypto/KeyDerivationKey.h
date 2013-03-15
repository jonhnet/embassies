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

#include "ZSymKey.h"
#include "SymEncKey.h"
#include "MacKey.h"
#include "PrfKey.h"
#include "ZPubKey.h"

class KeyDerivationKey : public PrfKey {
public:
	KeyDerivationKey (RandomSupply* rand) : PrfKey(rand) {;} 
	KeyDerivationKey (uint8_t* buffer, uint32_t len) : PrfKey(buffer, len) {;} 

	MacKey* deriveMacKey(uint8_t* tag, uint32_t tagLen);
	SymEncKey* deriveEncKey(uint8_t* tag, uint32_t tagLen);
	KeyDerivationKey* deriveKeyDerivationKey(uint8_t* tag, uint32_t tagLen);
	KeyDerivationKey* deriveKeyDerivationKey(ZPubKey* key);
	PrfKey* derivePrfKey(uint8_t* tag, uint32_t tagLen);

private:
	hash_t tagHash(uint8_t* tag, uint32_t tagLen);
	void deriveKey(uint8_t buffer[PrfKey::BLOCK_SIZE], hash_t tagHash);
};
