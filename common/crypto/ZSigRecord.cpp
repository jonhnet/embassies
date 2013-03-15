#include "array_alloc.h"
#include "LiteLib.h"
#include "ZSigRecord.h"
#include "ZKey.h"
#include "CryptoException.h"

/*
                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        Type Covered           |  Algorithm    |     Labels    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Original TTL                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Signature Expiration                     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Signature Inception                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            Key Tag            |                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         Signer's Name         /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                                                               /
   /                            Signature                          /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
ZSigRecord::ZSigRecord(uint8_t* sigRecord, uint32_t size) {
	sigRecord = parse(sigRecord, size);

  uint32_t const_size = sizeof(typeCovered) + sizeof(algorithm) + sizeof(labels) +
                        sizeof(originalTtl) + sizeof(expires) + sizeof(inception) + sizeof(keyTag);
	if (this->dataLength < const_size) {
		Throw(CryptoException::BAD_INPUT, "sigRecord too short");
	}

	PARSE_SHORT(sigRecord, this->typeCovered);
	PARSE_BYTE(sigRecord, this->algorithm);
	PARSE_BYTE(sigRecord, this->labels);
	PARSE_INT(sigRecord, this->originalTtl);
	PARSE_INT(sigRecord, this->expires);
	PARSE_INT(sigRecord, this->inception);
	PARSE_SHORT(sigRecord, this->keyTag);

	this->signerName = new DomainName(sigRecord, this->dataLength - const_size);
	if(!this->signerName) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate signerName"); }
	sigRecord += this->signerName->size();

	// Subtract off everything before the signature
	uint16_t sigLen = this->dataLength - const_size - this->signerName->size();
	this->sig = malloc_array(sigLen);
	if (!this->sig) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate signature"); }
	lite_memcpy(this->sig, sigRecord, sigLen);
}

uint32_t ZSigRecord::serialize(uint8_t* buffer) {
	lite_assert(buffer);

	// Let the parent handle the generic data
	uint32_t len = ((ZRecord*)this)->ZRecord::serialize(buffer);
	buffer += len;

	// Write out everything but the signature
	uint32_t headLen = this->serializeHead(buffer);
	buffer += headLen;

	// Subtract off everything before the signature
	uint16_t sigLen = this->dataLength - headLen;
	lite_memcpy(buffer, this->sig, sigLen);
	
	return len + this->dataLength;  // Length of record header plus length of ds-related data
}


// Write everything but the signature into the buffer and return number of bytes written
uint32_t ZSigRecord::serializeHead(uint8_t* buffer) {
	lite_assert(buffer);

	WRITE_SHORT(buffer, this->typeCovered);
	WRITE_BYTE(buffer, this->algorithm);
	WRITE_BYTE(buffer, this->labels);
	WRITE_INT(buffer, this->originalTtl);
	WRITE_INT(buffer, this->expires);
	WRITE_INT(buffer, this->inception);
	WRITE_SHORT(buffer, this->keyTag);


	this->signerName->serialize(buffer);
	buffer += this->signerName->size();

	return this->headerSize();
}

uint32_t ZSigRecord::headerSize() { 
  uint32_t const_size = sizeof(typeCovered) + sizeof(algorithm) + sizeof(labels) +
                        sizeof(originalTtl) + sizeof(expires) + sizeof(inception) + sizeof(keyTag);
	const_size += this->signerName->size();
	return const_size;
}

// typeCovered : What's being signed?
// label: Who owns the signature record?
ZSigRecord::ZSigRecord(uint16_t typeCovered, const DomainName* label, uint32_t inception, uint32_t expires) 
	: ZRecord(label, ZOOG_RECORD_TYPE_SIG) {
	if (! (typeCovered == ZRecord::ZOOG_RECORD_TYPE_BINARY ||
		   typeCovered == ZRecord::ZOOG_RECORD_TYPE_DELEGATION ||
		   typeCovered == ZRecord::ZOOG_RECORD_TYPE_KEY ||
		   typeCovered == ZRecord::ZOOG_RECORD_TYPE_SIG || 
		   typeCovered == ZRecord::ZOOG_RECORD_TYPE_KEYLINK) ) {
		Throw(CryptoException::BAD_INPUT, "Invalid sig record type");
	}

	if (expires < inception) {
		Throw(CryptoException::BAD_INPUT, "Signature will never be valid!");
	}

	this->typeCovered = typeCovered;
	this->algorithm = SIG_ALG_ID;
	this->labels = this->owner->numLabels();
	this->originalTtl = 86400;
	this->expires = expires;
	this->inception = inception;
	this->keyTag = 0;				// Filled in when signed
	this->signerName = new DomainName(" ", sizeof(" "));		// Filled in when signed
	lite_assert(this->signerName);
	this->sig = NULL;

	// Still need to update this after we sign
  uint32_t const_size = sizeof(typeCovered) + sizeof(algorithm) + sizeof(labels) +
                        sizeof(originalTtl) + sizeof(expires) + sizeof(inception) + sizeof(keyTag);
	this->addDataLength(const_size + this->signerName->size());	
}

ZSigRecord::~ZSigRecord() {
	delete this->signerName;
	free_array(this->sig);
}

void ZSigRecord::setSignerName(DomainName* name) { 
	// Remove the old signer name
	this->addDataLength(-1*(this->signerName->size()));
	delete this->signerName;

	// Add the new one
	this->signerName = new DomainName(name); 
	lite_assert(this->signerName);
	this->addDataLength(signerName->size());

	// The signer should also own the record
	this->_size -= this->owner->size();
	delete this->owner;

	this->owner = new DomainName(name);
	lite_assert(this->owner);
	this->_size += this->owner->size();

	this->labels = this->owner->numLabels();
}

void ZSigRecord::setSig(uint8_t* sig, uint32_t sigSize) {	
	lite_assert(sig); 
	lite_assert(sigSize <= ZKey::SIGNATURE_SIZE);

	if (this->sig) {	// TODO: This assumes all signatures are the same size!
		this->addDataLength(-1 * sigSize);		
		delete this->sig;
	}

	this->sig = malloc_array(sigSize);
	lite_memcpy(this->sig, sig, sigSize);

	this->addDataLength(sigSize);	
}


bool ZSigRecord::isEqual(const ZRecord* z) const {
	if (this->equal(z) && z->getType() == ZRecord::ZOOG_RECORD_TYPE_SIG) {
		ZSigRecord* r = (ZSigRecord*)z;
		if (this->typeCovered == r->typeCovered && 
			this->algorithm == r->algorithm &&
			this->labels == r->labels &&
			this->originalTtl == r->originalTtl &&
			this->expires == r->expires &&
			this->inception == r->inception &&
			this->keyTag == r->keyTag  &&
			*this->signerName == *r->signerName
			) {
      uint32_t const_size = sizeof(typeCovered) + sizeof(algorithm) + sizeof(labels) +
                            sizeof(originalTtl) + sizeof(expires) + sizeof(inception) + sizeof(keyTag);
			for (uint32_t i = 0; 
				 i < (uint32_t) (this->dataLength - const_size - this->signerName->size());
				 i++) {
				if (this->sig[i] != r->sig[i]) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

