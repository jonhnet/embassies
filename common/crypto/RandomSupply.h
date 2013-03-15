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

// An abstraction for a class that uses a PRNG to stretch incoming
// entropy, mixing in new entropy as it is made available.

// Implemented via block cipher in counter mode

class RandomSupply {
private:
	uint8_t  counter[BLOCK_CIPHER_SIZE/8];
	uint32_t round_keys[ROUND_KEY_SIZE];
	uint8_t  pool[BLOCK_CIPHER_SIZE/8]; // For non-block-size requests
	uint32_t pool_ptr;

public:
	static const uint32_t SEED_SIZE = SYM_KEY_BYTES;

	RandomSupply(uint8_t bytes[SEED_SIZE]);

	//void inject_entropy(uint8_t *bytes, uint32_t length);

	uint8_t get_random_byte();
	void get_random_bytes(uint8_t *out_bytes, uint32_t length);
};
