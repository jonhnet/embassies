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
#include "crypto.h"
#include "rsa.h"

// Default signature algorithm
#ifdef USE_SHA1
#define PKCS_SIG_ALG_ID RSA_SHA1
#elif  USE_SHA256
#define PKCS_SIG_ALG_ID RSA_SHA256	
#elif  USE_SHA512
#define PKCS_SIG_ALG_ID RSA_SHA512
#endif 


class ZKey {
public:
	static const uint32_t SIGNATURE_SIZE = PUB_KEY_SIZE / 8;

	ZKey() { owner = NULL; rsa_ctx = NULL; }

	// Create a key from one we previously serialized
	ZKey(uint8_t* serializedKey, uint32_t size);	

	// Create a key from an existing context
	ZKey(rsa_context* rsa_ctx, const DomainName* owner);

	DomainName* getOwner() { return owner; }
	virtual uint16_t getTag() = 0;	
	
protected:
	DomainName* owner;
	rsa_context* rsa_ctx;    // Key in a form we can use to do work		

	uint32_t serializeKey(uint8_t* buffer);
	void parseKey(uint8_t* buffer, uint32_t size);
	uint32_t keySize();	// Number of bytes needed to serialize the key
};
