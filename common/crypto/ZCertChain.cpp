#include "ZCertChain.h"
#include "CryptoException.h"
#include "ZRecord.h"

ZCertChain::ZCertChain() {
	this->numCerts = 0;
}

ZCertChain::ZCertChain(uint8_t* buffer, uint32_t size) {
	if (!buffer || size < sizeof(uint8_t)) { Throw(CryptoException::BAD_INPUT, "NULL buffer given for deserializing ZCertChain"); }
	
	PARSE_BYTE(buffer, this->numCerts);
	int sizeRemaining = size - sizeof(uint8_t);

	if (this->numCerts >  ZCertChain::MAX_NUM_CERTS) {
		Throw(CryptoException::BAD_INPUT, "Tried to parse a ZCertChain with too many certs!");
	}

	for (int i = 0; i < this->numCerts; i++) {
		if (sizeRemaining <= 0) { Throw(CryptoException::BAD_INPUT, "Not enough data left to read a cert"); }

		this->certs[i] = new ZCert(buffer, sizeRemaining);

		buffer += this->certs[i]->size();
		sizeRemaining -= this->certs[i]->size();
	}
}

void ZCertChain::addCert(ZCert* cert) {
	if (this->numCerts >= ZCertChain::MAX_NUM_CERTS) {
		Throw(CryptoException::BAD_INPUT, "Tried to add too many certs to a ZCertChain");
	}

	this->certs[this->numCerts] = cert;
	this->numCerts++;
}

uint32_t ZCertChain::size() const {
	uint32_t size = 0;
	size += sizeof(uint8_t);	// Specifies number of certs

	for (int i = 0; i < this->numCerts; i++) {
		size += this->certs[i]->size();
	}

	return size;
}

void ZCertChain::serialize(uint8_t* buffer) {
	if (!buffer) { Throw(CryptoException::BAD_INPUT, "NULL buffer given for serializing ZCertChain"); }

	WRITE_BYTE(buffer, this->numCerts);

	for (int i = 0; i < this->numCerts; i++) {
		this->certs[i]->serialize(buffer);
		buffer += this->certs[i]->size();
	}
}

ZCert* ZCertChain::getMostSpecificCert() {
	if (this->numCerts <= 0) {
		return NULL;
	} else {
		return this->certs[this->numCerts-1];
	}
}

ZCert*& ZCertChain::operator[] (const int index) {
	if (index >= this->numCerts || index < 0) {
		Throw(CryptoException::BUFFER_OVERFLOW, "Asked for a non-existent cert from the cert chain");
	}

	return this->certs[index];
}
