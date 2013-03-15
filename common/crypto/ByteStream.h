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

#include "types.h"

class ByteStream {
public:
	// Assumes memory region [buffer, buffer+size) is valid
	// and remains valid for the lifetime of the ByteStream
	// Does not make a local copy!
	ByteStream(uint8_t* buffer, uint32_t size);

	friend ByteStream& operator<<(ByteStream& out, uint8_t value);
	friend ByteStream& operator<<(ByteStream& out, uint16_t value);
	friend ByteStream& operator<<(ByteStream& out, uint32_t value);

	friend ByteStream& operator>>(ByteStream& in, uint8_t  &value);
	friend ByteStream& operator>>(ByteStream& in, uint16_t &value);
	friend ByteStream& operator>>(ByteStream& in, uint32_t &value);

	uint8_t& operator[] (const int index); 

private:
	uint8_t* buffer;
	uint32_t size;
	uint32_t pos;

	// Assert that there are at least numBytes remaining in the buffer
	void assertBytesRemaining(uint32_t numBytes);

	// Assert that index < size
	void assertInBounds(uint32_t index);
};
