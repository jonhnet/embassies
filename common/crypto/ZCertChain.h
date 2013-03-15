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

// Class for bundling a series of certs together

#include "ZCert.h"

class ZCert;

class ZCertChain {
public:
	static const uint8_t MAX_NUM_CERTS = 10;

	ZCertChain();
	ZCertChain(uint8_t* buffer, uint32_t size);

	void addCert(ZCert* cert);

	uint32_t size() const;
	void serialize(uint8_t* buffer);

	uint8_t getNumCerts() { return numCerts; }
	ZCert* getMostSpecificCert(); 

	ZCert*& operator[] (const int index); 

private:
	uint8_t numCerts;
	ZCert* certs[MAX_NUM_CERTS];
};
