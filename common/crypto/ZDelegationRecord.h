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

// Derived from the DS resource record
class ZDelegationRecord : public ZRecord {
public:
	ZDelegationRecord(uint8_t* dsRecord, uint32_t size);
	~ZDelegationRecord();

	// Write the record into the buffer and return number of bytes written
	uint32_t serialize(uint8_t* buffer);

	bool isEqual(const ZRecord* z) const;

private:
	uint16_t keyTag;		// Hint as to which key we're delegating to
	uint8_t	 algorithm;		// Must be one of ZOOG_SIGN_ALGORITHM_*
	uint8_t	 digestType;	// Must be one of ZOOG_DIGEST_*
	uint8_t* digest;		// Digest of the zkey
};