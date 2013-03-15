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
#include "ZPubKeyRecord.h"

// Describes a Zoog boot keySpeaker
// Derived from the TXT resource record
class ZKeyLinkRecord : public ZRecord {
public:
	ZKeyLinkRecord(uint8_t* txtRecord, uint32_t size);
	ZKeyLinkRecord(ZPubKey* keySpeaker, ZPubKey* keySpokenFor);
	~ZKeyLinkRecord();

	// Write the record into the buffer and return number of bytes written
	uint32_t serialize(uint8_t* buffer);

	bool isEqual(const ZRecord* z) const;

	ZPubKey* getSpeaker() { return keySpeaker->getKey(); }
	ZPubKey* getSpokenFor() { return keySpokenFor->getKey(); }

private:
	ZPubKeyRecord* keySpeaker;
	ZPubKeyRecord* keySpokenFor;
};
