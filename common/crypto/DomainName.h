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
#include "ByteStream.h"

class DomainName {
public:
	static const uint8_t MAX_DNS_NAME_LEN = 255;
	
	/* Duplicate the DomainName provided */
	DomainName(const DomainName* name);
	
	/* 
	 * Expects a DNSSEC-style byte array consisting of one or more labels.
	 * Each label begins with a 1-byte count of the number of 
	 * bytes that follows.  Name should be terminated by a byte of 0
	 * (a label with length 0), which is the root label
	 * Assumes the memory range [name, name+size) is valid
	 * (but name may be shorter than size)
	 */
	DomainName(const uint8_t* dnsbuffer, const uint32_t size);

	/* 
	 * Expects a buffer containing a DNSSEC-style byte array consisting of one or more labels.
	 * Each label begins with a 1-byte count of the number of 
	 * bytes that follows.  Name should be terminated by a byte of 0
	 * (a label with length 0), which is the root label
	 * Assumes the memory range [name, name+size) is valid
	 * (but name may be shorter than size)
	 */
	DomainName(ByteStream &buffer);

	/*
	 * Build a domain name object from a domain name string
	 * e.g., www.bing.com.
	 * Assumes the memory range [name, name+size) is valid
	 * and includes the entire name (characters plus null terminator)
	 */
	DomainName(const char* name, const uint32_t size);

	// Number of bytes needed to serialize this name
	uint32_t size() const;

	// Write the record into the buffer and return number of bytes written
	// Assumes buffer is at least size() long
	uint32_t serialize(uint8_t* buffer) const;
	uint32_t serialize(ByteStream &buffer) const;
	
	// Number of labels, not counting the root or any wildcards
	uint8_t numLabels() const;

	friend bool operator==(DomainName &d1, DomainName &d2);
	friend bool operator!=(DomainName &d1, DomainName &d2);

	// Return true if this DomainName has name for a suffix
	bool suffix(const DomainName* name) const;

	char* toString();

	// Determine if this is a valid DNS domain name
	static bool checkDomainName(const char* name, uint32_t len);

private:
	void parse(const char* name, const uint32_t len);

	// Internally, we store the name as a null-terminated dotted string
	char nameBytes[MAX_DNS_NAME_LEN];
	uint16_t len;
	uint8_t num_labels;
};