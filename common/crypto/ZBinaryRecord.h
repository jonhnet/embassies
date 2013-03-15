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

// Describes a Zoog boot block
// Derived from the TXT resource record
class ZBinaryRecord : public ZRecord {
public:
	// Use foo to distinguish this derserialization constructor.  
	// Plan is replace all of these arguments with a SerializedStream class
	ZBinaryRecord(uint8_t* txtRecord, uint32_t size, int foo);

	ZBinaryRecord(const uint8_t* binary, uint32_t binaryLen);	
	~ZBinaryRecord();

	// Write the record into the buffer and return number of bytes written
	uint32_t serialize(uint8_t* buffer);

	bool isEqual(const ZRecord* z) const;

	//friend bool operator==(ZBinaryRecord &r1, ZBinaryRecord &r2);
	//friend bool operator!=(ZBinaryRecord &r1, ZBinaryRecord &r2);

private:
	uint16_t flags;			// Reserved
	uint16_t digestType;	// Must be one of ZOOG_DIGEST_*
	uint8_t* digest;		// Digest of the boot block
};
