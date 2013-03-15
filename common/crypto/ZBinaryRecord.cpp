//#include <assert.h>
#ifndef _WIN32
//#include <string.h>
#endif // !_WIN32

#include "LiteLib.h"
#include "array_alloc.h"
#include "ZBinaryRecord.h"
#include "ZCert.h"
#include "crypto.h"
#include "CryptoException.h"
#include "zoog_network_order.h"

/*
                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              Flags            |             digestType        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /															   /
   /                             digest                            /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
ZBinaryRecord::ZBinaryRecord(uint8_t* txtRecord, uint32_t size, int foo) {
	if(!txtRecord) { Throw(CryptoException::BAD_INPUT, "Null record"); }
	foo = 1; // Avoid compiler warning about unused variable

	// Parse the generic DNS record info
	txtRecord = parse(txtRecord, size);

	// Now get the binary specific info
	PARSE_SHORT(txtRecord, this->flags);
	PARSE_SHORT(txtRecord, this->digestType);
		
	// Subtract off flags and digestType
	uint16_t digestLen = this->dataLength - 2 - 1;
	this->digest = malloc_array(digestLen);
	if(!this->digest) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate digest"); }
	lite_memcpy(this->digest, txtRecord, digestLen);
}

// Write the record into the buffer and return number of bytes written
uint32_t ZBinaryRecord::serialize(uint8_t* buffer) {
	if(!buffer) { Throw(CryptoException::BAD_INPUT, "Null buffer"); }

	// Let the parent handle the generic data
	uint32_t len = ((ZRecord*)this)->ZRecord::serialize(buffer);
	buffer += len;

	// Now write the binary specific info
	WRITE_SHORT(buffer, this->flags);
	WRITE_SHORT(buffer, this->digestType);
	
	// Subtract off flags and digestType
	uint16_t digestLen = this->dataLength - sizeof(flags) - sizeof(digestType);
	lite_memcpy(buffer, this->digest, digestLen);
	
	return len + this->dataLength;  // Length of record header plus length of binary-related data
}

ZBinaryRecord::ZBinaryRecord(const uint8_t* binary, uint32_t binaryLen) 
	: ZRecord(NULL, ZOOG_RECORD_TYPE_BINARY ) {
	this->flags = 0;
	this->digestType = ZOOG_DIGEST_DEFAULT;

	this->digest = malloc_array(HASH_SIZE);
	if(!this->digest ) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate digest"); }

	zhash(binary, binaryLen, (hash_t*)this->digest);

	this->addDataLength(sizeof(flags) + sizeof(digestType) + HASH_SIZE);  
}

bool ZBinaryRecord::isEqual(const ZRecord* z) const {
	if (this->equal(z) && z->getType() == ZRecord::ZOOG_RECORD_TYPE_BINARY) {
		ZBinaryRecord* r = (ZBinaryRecord*)z;
		if (this->flags == r->flags && this->digestType == r->digestType) {
			for (uint32_t i = 0; i < (uint32_t) (this->dataLength - sizeof(flags) - sizeof(digestType)); i++) {
				if (this->digest[i] != r->digest[i]) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

//bool operator==(ZBinaryRecord &r1, ZBinaryRecord &r2) {
//	if (r1.equal(&r2) && r1.flags == r2.flags && r1.digestType == r2.digestType) {
//		for (uint32_t i = 0; i < (uint32_t) (r1.dataLength - 4); i++) {
//			if (r1.digest[i] != r2.digest[i]) {
//				return false;
//			}
//		}
//		return true;
//	} 
//	return false;		
//}
//
//bool operator!=(ZBinaryRecord &r1, ZBinaryRecord &r2) {
//	return !(r1 == r2);
//}

ZBinaryRecord::~ZBinaryRecord() {
	free_array(this->digest);
}

