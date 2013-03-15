#include "array_alloc.h"
#include "LiteLib.h"
#include "ZPubKeyRecord.h"
#include "ZSigRecord.h"
#include "CryptoException.h"

/*
                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              Flags            |    Protocol   |   Algorithm   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                                                               /
   /                            Public Key                         /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Note that the protocol field is always 3.
*/

ZPubKeyRecord::ZPubKeyRecord(uint8_t* dnskey, uint32_t size) {
	// Parse the generic DNS record info
	dnskey = parse(dnskey, size);

	// Now get the key specific info
	PARSE_SHORT(dnskey, this->flags);
	PARSE_BYTE(dnskey, this->protocol);
	PARSE_BYTE(dnskey, this->algorithm);
	
	// Subtract off flags, protocol, algorithm
	lite_assert(this->dataLength>=4);
	uint16_t keyLen = this->dataLength - 2 - 1 - 1;
	this->key = malloc_array(keyLen);
	lite_assert(this->key);
	lite_memcpy(this->key, dnskey, keyLen);

	this->pubkey = NULL;
}

uint32_t ZPubKeyRecord::serialize(uint8_t* buffer) {
	lite_assert(buffer);

	// Let the parent handle the generic data
	uint32_t len = ((ZRecord*)this)->ZRecord::serialize(buffer);
	buffer += len;

	// Now do the key-specific parts
	serializeKey(buffer);
	
	return len + this->dataLength;  // Length of record header plus length of key-related data
}

uint32_t ZPubKeyRecord::serializeKey(uint8_t* buffer) {
	lite_assert(buffer);

	// Now get the key specific info
	WRITE_SHORT(buffer, this->flags);
	WRITE_BYTE(buffer, this->protocol);
	WRITE_BYTE(buffer, this->algorithm);

	// Subtract off flags, protocol, algorithm
	uint16_t keyLen = this->dataLength - 2 - 1 - 1;
	lite_memcpy(buffer, this->key, keyLen);
	
	return this->dataLength;  
}

// Copy an existing PubKey record
ZPubKeyRecord::ZPubKeyRecord(ZPubKeyRecord* key) {
	this->owner = new DomainName(key->owner);
	lite_assert(this->owner);
	this->type = key->type;
	this->recordClass = key->recordClass;
	this->ttl = key->ttl;
	this->dataLength = key->dataLength;
	this->_size = key->_size;

	this->flags = key->flags;
	this->protocol = key->protocol;
	this->algorithm = key->algorithm;

	if (key->pubkey == NULL) {
		this->pubkey = NULL;
	} else {
		this->pubkey = new ZPubKey(key->pubkey);
	}

	uint32_t keySize = this->dataLength - sizeof(flags) - sizeof(protocol) - sizeof(algorithm);
	this->key = malloc_array(keySize);
	lite_assert(this->key);
	lite_memcpy(this->key, key->key, keySize);
}

ZPubKeyRecord::ZPubKeyRecord(const DomainName* label, uint8_t* keyBytes, uint32_t keySize) 
	: ZRecord(label, ZOOG_RECORD_TYPE_KEY ) {
	this->flags = 256; // Set bit 7 to indicate this is a Zone Key  // TODO: Check this
	this->protocol = 3; // Always 3
	this->algorithm = SIG_ALG_ID;

	this->key = malloc_array(keySize);
	lite_memcpy(this->key, keyBytes, keySize);	

	this->pubkey = NULL;

	this->addDataLength(sizeof(flags) + sizeof(protocol) + sizeof(algorithm) + keySize);
}

ZPubKeyRecord::~ZPubKeyRecord() {
	free_array(this->key);
	delete this->pubkey;
}

// Returns a usuable key
ZPubKey* ZPubKeyRecord::getKey() {
	if (this->pubkey == NULL) {
		uint32_t keySize = this->dataLength - sizeof(flags) - sizeof(protocol) - sizeof(algorithm);
		this->pubkey = new ZPubKey(this->algorithm, this->key, keySize, this->owner);
		lite_assert(this->pubkey);
	}

	return this->pubkey;
}

// Get a handy tag for identifying this key (not guaranteed to be unique!)
// Algorithm adapted from RFC 4034
uint16_t ZPubKeyRecord::getTag() {
    unsigned long ac;     /* assumed to be 32 bits or larger */
    int i;                /* loop index */
	uint8_t* buffer = malloc_array(this->dataLength);
	this->serializeKey(buffer);

    for ( ac = 0, i = 0; i < this->dataLength; ++i )
            ac += (i & 1) ? buffer[i] : buffer[i] << 8;
    ac += (ac >> 16) & 0xFFFF;
	free_array(buffer);
    return ac & 0xFFFF;
}


bool ZPubKeyRecord::isEqual(const ZRecord* z) const {
	if (this->equal(z) && z->getType() == ZRecord::ZOOG_RECORD_TYPE_KEY) {
		ZPubKeyRecord* r = (ZPubKeyRecord*)z;
		if (this->flags == r->flags && 
			this->protocol == r->protocol &&
			this->algorithm == r->algorithm ) {
			// Note that this->pubkey is used for caching, so we don't compare it
			for (uint32_t i = 0; i < (uint32_t) (this->dataLength - 4); i++) {
				if (this->key[i] != r->key[i]) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}
