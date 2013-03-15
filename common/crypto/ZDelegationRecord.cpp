//#include <assert.h>
#ifndef _WIN32
//#include <string.h>
#endif // !_WIN32

#include "array_alloc.h"
#include "LiteLib.h"
#include "ZDelegationRecord.h"
#include "CryptoException.h"

/*
                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Key Tag             |  Algorithm    |  Digest Type  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                                                               /
   /                            Digest                             /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
ZDelegationRecord::ZDelegationRecord(uint8_t* dsRecord, uint32_t size) {
	dsRecord = parse(dsRecord, size);

	uint32_t const_size = sizeof(keyTag) + sizeof(algorithm) + sizeof(digestType);
	if ((uint32_t)(this->dataLength + const_size) < size) {
		Throw(CryptoException::BAD_INPUT, "Delegation record too short");
	}

	// Now get the ds specific info
	PARSE_SHORT(dsRecord, this->keyTag);
	PARSE_BYTE(dsRecord, this->algorithm);
	PARSE_BYTE(dsRecord, this->digestType);

	// Subtract off key tag, algorithm, and digest type
	uint16_t digestLen = this->dataLength - const_size;
	this->digest = malloc_array(digestLen);
	lite_assert(this->digest);
	lite_memcpy(this->digest, dsRecord, digestLen);
}

uint32_t ZDelegationRecord::serialize(uint8_t* buffer) {
	lite_assert(buffer);

	// Let the parent handle the generic data
	uint32_t len = ((ZRecord*)this)->ZRecord::serialize(buffer);
	buffer += len;

	// Now write the ds specific info
	WRITE_SHORT(buffer, this->keyTag);
	WRITE_BYTE(buffer, this->algorithm);
	WRITE_BYTE(buffer, this->digestType);

	// Subtract off key tag, algorithm, and digest type
	uint16_t digestLen = this->dataLength - 2 - 1 - 1;
	lite_memcpy(buffer, this->digest, digestLen);
	
	return len + this->dataLength;  // Length of record header plus length of ds-related data
}

bool ZDelegationRecord::isEqual(const ZRecord* z) const {
	if (this->equal(z) && z->getType() == ZRecord::ZOOG_RECORD_TYPE_DELEGATION) {
		ZDelegationRecord* r = (ZDelegationRecord*)z;
		if (this->keyTag == r->keyTag && 
			this->algorithm == r->algorithm && 
			this->digestType == r->digestType) {
			for (uint32_t i = 0; i < (uint32_t) (this->dataLength - 4); i++) {
				if (this->digest[i] != r->digest[i]) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

ZDelegationRecord::~ZDelegationRecord() {
	free_array(this->digest);
}
