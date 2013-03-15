#include "ZRecord.h"
#include "ZSigRecord.h"
#include "ZPubKeyRecord.h"
#include "ZBinaryRecord.h"
#include "ZKeyLinkRecord.h"
#include "ZDelegationRecord.h"
#include "CryptoException.h"

ZRecord::ZRecord() {
	this->owner = NULL;
	this->type = 0;
	this->recordClass = 1;
	this->ttl = 0;
	this->dataLength = 0;
	this->_size = 0;
}

ZRecord::ZRecord(const DomainName* label, uint16_t type) {
	if (label == NULL) {
		// No label specified so use a generic one
		this->owner = new DomainName((char*)"ZOOG", sizeof("ZOOG"));
	} else {
		this->owner = new DomainName(label);
	}
	this->type = type;
	this->recordClass = 1;
	this->ttl = 86400;
	this->dataLength = 0;
	this->_size = this->owner->size() + sizeof(type) + sizeof(recordClass) + sizeof(ttl) + sizeof(dataLength);
}

/*
                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                                                               /
   /                            Domain Name                        /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              Type             |             Class             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                              TTL                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           data length         |                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               /
   /                             Data                              /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
uint8_t* ZRecord::parse(uint8_t* record, uint32_t size) {
	if(!record) { Throw(CryptoException::BAD_INPUT, "Null record"); }
	this->owner = new DomainName(record, size);

	if(!this->owner) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate owner"); }
	record += this->owner->size();

	uint8_t const_size = sizeof(type) + sizeof(recordClass) + sizeof(ttl) + sizeof(dataLength);
	if(! ((uint32_t) (this->owner->size() + const_size) <= size)) {
		Throw(CryptoException::BAD_INPUT, "ZRecord too short");
	}

	PARSE_SHORT(record, this->type);
	PARSE_SHORT(record, this->recordClass);
	PARSE_INT(record, this->ttl);
	PARSE_SHORT(record, this->dataLength);
	
	this->_size = this->owner->size() + const_size + this->dataLength;

	if(! (this->_size <= size)) {
		Throw(CryptoException::BAD_INPUT, "ZRecord too short");
	}

	return record;
}

// Write the record into the buffer and return number of bytes written
// Does not write out child's data
uint32_t ZRecord::serialize(uint8_t* buffer) {
	if(!buffer) { Throw(CryptoException::BAD_INPUT, "Null buffer"); }

	uint32_t len = this->owner->serialize(buffer);
	buffer += len;

	WRITE_SHORT(buffer, this->type);
	WRITE_SHORT(buffer, this->recordClass);
	WRITE_INT(buffer, this->ttl);
	WRITE_SHORT(buffer, this->dataLength);

	return len + 2 + 2 + 4 + 2;
}

// Create a new subrecord of the appropriate type
ZRecord* ZRecord::spawn(uint8_t* record, uint32_t size) {
	if(!record) { Throw(CryptoException::BAD_INPUT, "NULL record"); }
 
	// Parse past the initial domain name to get to the record type
	DomainName* owner = new DomainName(record, size);
	if(!owner) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate owner"); }

	if ((uint32_t)(owner->size() + 2) > size) { Throw(CryptoException::BAD_INPUT, "Record too short"); }
	uint16_t type = z_ntohs(*(uint16_t*) (record + owner->size()) );

	delete owner;

	switch (type) {
		case ZRecord::ZOOG_RECORD_TYPE_BINARY: 
			return new ZBinaryRecord(record, size, 1);

		case ZRecord::ZOOG_RECORD_TYPE_SIG:
			return new ZSigRecord(record, size);

		case ZRecord::ZOOG_RECORD_TYPE_KEY:
			return new ZPubKeyRecord(record, size);

		case ZRecord::ZOOG_RECORD_TYPE_DELEGATION:
			return new ZDelegationRecord(record, size);

		case ZRecord::ZOOG_RECORD_TYPE_KEYLINK:
			return new ZKeyLinkRecord(record, size);

		default:
			return NULL;
	}
}

// Assess whether these two records are equal based on their generic record fields
// Note that the subclass data may still be different!
bool ZRecord::equal(const ZRecord* z) const {
	return z && this->owner && z->owner &&
		   *this->owner == *z->owner &&
		   this->type == z->type && 
		   this->recordClass == z->recordClass && 
		   this->ttl == z->ttl && 
		   this->dataLength == z->dataLength && 
		   this->_size == z->_size;
}


ZRecord::~ZRecord() {
	delete this->owner;
}
