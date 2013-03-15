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

/*
// Need these for ntoh* and hton*
#ifdef _WIN32
#include <Winsock2.h>
#else 
#include <netinet/in.h>
#endif // _WIN32
*/
// sorry, those aren't safe for challenging environments (e.g. Genode)
// Use the zoog versions instead.
#include "zoog_network_order.h"

#include "types.h"
#include "DomainName.h"
#include "crypto.h"

/* Creates warnings re: untaken branches don't have the right size
#define PARSE(buffer, target) \
	if (sizeof(target) == 4) { \
		(target) = z_ntohl(*(uint32_t*) (buffer)); \
	} else if (sizeof(target) == 2) { \
		(target) = z_ntohs(*(uint16_t*) (buffer)); \
	} else if (sizeof(target) == 1) { \
		(target) = *(buffer); \
	} else { \
		Throw(CryptoException::BAD_INPUT, "Unknown parse size"); \
	} \
	(buffer) += sizeof(target);
*/

#define PARSE_INT(buffer, target) \
	(target) = z_ntohl(*(uint32_t*) (buffer)); \
	(buffer) += 4;

#define PARSE_SHORT(buffer, target) \
	(target) = z_ntohs(*(uint16_t*) (buffer)); \
	(buffer) += 2;

#define PARSE_BYTE(buffer, target) \
	(target) = *(buffer); \
	(buffer) += 1;

/*
#define WRITE(buffer, source) \
	if (sizeof(source) == 4) { \
		*(uint32_t*) (buffer) = z_htonl(source); \
	} else if (sizeof(source) == 2) { \
		*(uint16_t*) (buffer) = z_htons(source); \
	} else if (sizeof(source) == 1) { \
		*(buffer) = (source); \
	} else { \
		Throw(CryptoException::BAD_INPUT, "Unknown write size"); \
	} \
	(buffer) += sizeof(source);
*/

#define WRITE_INT(buffer, source) \
	*(uint32_t*) (buffer) = z_htonl(source); \
	(buffer) += 4;

#define WRITE_SHORT(buffer, source) \
	*(uint16_t*) (buffer) = z_htons(source); \
	(buffer) += 2;

#define WRITE_BYTE(buffer, source) \
	*(buffer) = (source); \
	(buffer) += 1;

class ZRecord {
public:
	// Record types
	static const uint16_t ZOOG_RECORD_TYPE_BINARY		= 16;	/* Corresponds to TXT */
	static const uint16_t ZOOG_RECORD_TYPE_SIG			= 43;	/* Corresponds to RRSIG */
	static const uint16_t ZOOG_RECORD_TYPE_KEY			= 46;	/* Corresponds to DNSKEY */
	static const uint16_t ZOOG_RECORD_TYPE_DELEGATION   = 48;	/* Corresponds to DS */
	static const uint16_t ZOOG_RECORD_TYPE_KEYLINK		= 64;	/* No DNS correspondence.  TODO: Combine with TXT */

	// Create a new subrecord of the appropriate type
	static ZRecord* spawn(uint8_t* record, uint32_t size);
	uint8_t* parse(uint8_t* record, uint32_t size);

	// Write the record into the buffer and return number of bytes written
	virtual uint32_t serialize(uint8_t* buffer);	

	// Size of the entire record, including data
	uint32_t size() { return _size; }

	// Call when expanding/contracting the amount of data stored
	void addDataLength(short len) {  dataLength += len; _size += len; }

	uint16_t getType() const { return type; }

	virtual bool isEqual(const ZRecord* z) const = 0;

	virtual ~ZRecord();
protected:
	ZRecord();
	ZRecord(const DomainName* label, uint16_t type);

	// Assess whether these two records are equal based on their generic record fields
	// Note that the subclass data may still be different!
	bool equal(const ZRecord* z) const;

	DomainName* owner;
	uint16_t type;			// Must be one of ZOOG_RECORD_TYPE_*
	uint16_t recordClass;	// Typically 1
	uint32_t ttl;			
	uint16_t dataLength;
	uint32_t _size;			// Size of the entire record
};
