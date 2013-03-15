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
#include "PrfKey.h"
#include "MacKey.h"
#include "RandomSupply.h"

typedef struct {
	uint32_t len;
	Mac mac;	// We always use authenticated encryption
	uint8_t IV[BLOCK_CIPHER_SIZE/8];
	uint8_t bytes[0];
} Ciphertext;

typedef struct {
	uint32_t len;
	uint8_t bytes[0];
} Plaintext;

class SymEncKey : public PrfKey {
public:
	SymEncKey(RandomSupply* rand);
	SymEncKey(uint8_t* buffer, uint32_t len);
	~SymEncKey();

	// Encrypts the buffer and returns a len+buffer of ciphertext 
	Ciphertext* encrypt(RandomSupply* rand, uint8_t* buffer, uint32_t len);
	
	// Expects a len+buffer of ciphertext and returns a len+buffer of plaintext
	// Returns NULL if decryption fails
	Plaintext* decrypt(Ciphertext* cipher);

private:
	MacKey* macKey;		// Used for authentication

	void CtrMode(uint8_t* input, uint32_t len, uint8_t IV[BLOCK_CIPHER_SIZE/8], uint8_t* output);
};
