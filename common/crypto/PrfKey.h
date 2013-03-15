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

class PrfKey : public ZSymKey {
public:
	static const uint32_t BLOCK_SIZE = 16;
	
	PrfKey(RandomSupply* rand); 
	PrfKey(uint8_t* buffer, uint32_t len);

	void evaluatePrf(uint8_t input[BLOCK_SIZE], uint8_t output[BLOCK_SIZE]);

protected:
	uint32_t roundKeys[ROUND_KEY_SIZE];
	uint8_t getNumAesRounds();

};
