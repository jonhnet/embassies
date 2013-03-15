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

#include "crypto.h"
#include "RandomSupply.h"

class ZSymKey {
public:
	// Generates a fresh random key of specified size
	ZSymKey(RandomSupply* random_supply, uint32_t numBits=SYM_KEY_BITS);

	// Duplicates an existing key 
	ZSymKey(uint8_t* buffer, uint32_t len);

	~ZSymKey();

	uint8_t* getBytes();
	uint32_t getNumBits();
	uint32_t getNumBytes();

	// Requires bufferLen >= size()
	void serialize(uint8_t* buffer, uint32_t bufferLen);
	uint32_t size();

protected:
	uint8_t* bytes;
	uint32_t numBits;

	virtual void init(uint8_t* buffer, uint32_t numBits);
};
