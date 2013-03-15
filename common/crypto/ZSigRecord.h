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

// Default signature algorithm
#ifdef USE_SHA1
#define SIG_ALG_ID ZSigRecord::ZOOG_SIGN_ALGORITHM_RSA_SHA_1
#elif  USE_SHA256
#define SIG_ALG_ID ZSigRecord::ZOOG_SIGN_ALGORITHM_RSA_SHA_256	
#elif  USE_SHA512
#define SIG_ALG_ID ZSigRecord::ZOOG_SIGN_ALGORITHM_RSA_SHA_512
#endif 

// Derived from the RRSIG resource record
class ZSigRecord : public ZRecord {
public:	
	// Types of signature algorithms available
	static const uint8_t ZOOG_SIGN_ALGORITHM_RSA_SHA_1		= 5;	/* Consistent with RFC 4034, A.1 */
	static const uint8_t ZOOG_SIGN_ALGORITHM_RSA_SHA_256	= 8;	/* Consistent with proposed RFC 5702 */
	static const uint8_t ZOOG_SIGN_ALGORITHM_RSA_SHA_512	= 10;   /* Consistent with proposed RFC 5702 */

	ZSigRecord(uint8_t* sigRecord, uint32_t size);

	// typeCovered : What's being signed?
	// label: Who owns the signature record?
	ZSigRecord(uint16_t typeCovered, const DomainName* label, uint32_t inception, uint32_t expires);
	~ZSigRecord();

	// Write the record into the buffer and return number of bytes written
	uint32_t serialize(uint8_t* buffer);

	// Write everything but the signature into the buffer and return number of bytes written
	// Assumes buffer is at least size() bytes
	uint32_t serializeHead(uint8_t* buffer);

	uint32_t headerSize();

	uint32_t getTypeCovered() { return typeCovered ;}
	uint32_t getInception() { return inception ;}
	uint32_t getExpires() { return expires ;}
	uint32_t getNumLabels() { return labels ;}

	void setKeyTag(uint16_t tag) { this->keyTag = tag; }
	void setSignerName(DomainName* name);
	DomainName* getSignerName() { return signerName; }

	void setSig(uint8_t* sig, uint32_t sigSize);
	uint8_t* getSig() { return sig; }

	bool isEqual(const ZRecord* z) const;

private:
	uint16_t typeCovered;	// Type of record signed
	uint8_t	 algorithm;		// Must be one of ZOOG_SIGN_ALGORITHM_*
	uint8_t  labels;		// # of labels in the original RRSIG RR owner name
	uint32_t originalTtl;
	uint32_t expires;		// Not valid after this date (in Unix time)
	uint32_t inception;		// Not valid before this date (in Unix time)
	uint16_t keyTag;		// Hint as to which key signed this
	DomainName* signerName;	
	uint8_t* sig;
};
