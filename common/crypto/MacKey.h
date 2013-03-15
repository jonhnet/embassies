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
#include "RandomSupply.h"

#if __GNUC__
#define ALIGN(n)      __attribute__ ((aligned(n))) 
#elif _MSC_VER
#define ALIGN(n)      __declspec(align(n))
#else
#define ALIGN(n)
#endif

typedef struct {
	uint32_t prefixOverhang;
	uint32_t postfixOverhang;
	uint8_t nonce[16];
	uint8_t mac[VMAC_TAG_LEN/8];
} Mac;

class MacKey : public ZSymKey {
public:
	MacKey(RandomSupply* rand); 
	MacKey (uint8_t* buffer, uint32_t len); 

	Mac* mac(RandomSupply* random_supply, uint8_t* buffer, uint32_t len);
	bool verify(Mac* mac, uint8_t* buffer, uint32_t len);

private:
	ALIGN(16) vmac_ctx_t ctx;
	Mac* computeMac(uint8_t* buffer, uint32_t len, uint8_t nonce[16]);
};
