//#include <assert.h>
//#include <stdio.h>
#include "array_alloc.h"
#include "LiteLib.h"
#include "ZCert.h"
#include "crypto.h"
#include "ZBinaryRecord.h"
#include "ZKeyLinkRecord.h"
#include "CryptoException.h"
#include "ZPrivKey.h"

/*
static void dump_bytes(unsigned char *bytes, int len) {
    int i;
    if(!bytes) return;

    for (i=0; i<len; i++) {
        printf("%02x", bytes[i]);
        if(i>0 && !((i+1)%16)) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
    if(len%16) {
        printf("\n");
    }
}
*/


// Read an entire certificate in
// Format is: objectLength, object, sigLength, sig
ZCert::ZCert(uint8_t* serializedCert, uint32_t size) {
	if (!serializedCert) { Throw(CryptoException::BAD_INPUT, "Null serializedCert"); }

	uint32_t recordSize = 0;
	if (size < 4) { Throw(CryptoException::BAD_INPUT, "Cert too short"); }
	PARSE_INT(serializedCert, recordSize);
	size -= 4;
	if (recordSize != 0) {
		this->object = ZRecord::spawn(serializedCert, size);
		if(!this->object) { Throw(CryptoException::OUT_OF_MEMORY, "Failed to allocate object"); }
		serializedCert += this->object->size();
		size -= this->object->size();
	} else {
		this->object = NULL;
	}

	if (size < 4) { Throw(CryptoException::BAD_INPUT, "Cert too short"); }
	PARSE_INT(serializedCert, recordSize);
	size -= 4;
	if (recordSize != 0) {
		this->sig = new ZSigRecord(serializedCert, size);
		lite_assert(sig);
		serializedCert += this->sig->size();
		size -= this->sig->size();
	} else {
		this->sig = NULL;
	}

	if (size < 4) { Throw(CryptoException::BAD_INPUT, "Cert too short"); }
	PARSE_INT(serializedCert, recordSize);
	size -= 4;
	if (recordSize != 0) {
		this->signingKey = new ZPubKeyRecord(serializedCert, size);
		lite_assert(signingKey);
		serializedCert += this->signingKey->size();
		size -= this->signingKey->size();
	} else {
		this->signingKey = NULL;
	}
}

ZCert::ZCert(uint8_t* record, uint32_t rsize, 
			 uint8_t* sigRecord, uint32_t ssize,
			 uint8_t* signingKeyRecord, uint32_t ksize) {
  this->sig = new ZSigRecord(sigRecord, rsize);
  this->object = ZRecord::spawn(record, ssize);
  this->signingKey = new ZPubKeyRecord(signingKeyRecord, ksize);
}

// Initialize a certificate describing a binding from a key to a name
// Semantics: The key that signs this cert says <<key speaksfor the name in label>>
ZCert::ZCert(uint32_t inception, uint32_t expires, const DomainName* label, ZPubKey* key) {
	this->object = new ZPubKeyRecord(key->getRecord());
	this->sig = new ZSigRecord(ZRecord::ZOOG_RECORD_TYPE_KEY, label, inception, expires);
	this->signingKey = NULL;
}

// Initialize a certificate for a binary
// Semantics: The key that signs this cert says <<binary speaksfor me>> 
ZCert::ZCert(uint32_t inception, uint32_t expires, const uint8_t* binary, uint32_t binaryLen) {
	this->object = new ZBinaryRecord(binary, binaryLen);
	this->sig = new ZSigRecord(ZRecord::ZOOG_RECORD_TYPE_BINARY, NULL, inception, expires);
	this->signingKey = NULL;
}

// Initialize a certificate describing a binding from keyA to keyB
// Semantics: The key that signs this cert says <<keySpeaker speaksfor keySpokenFor>>
ZCert::ZCert(uint32_t inception, uint32_t expires, ZPubKey* keySpeaker, ZPubKey* keySpokenFor) {
	this->object = new ZKeyLinkRecord(keySpeaker, keySpokenFor);
	this->sig = new ZSigRecord(ZRecord::ZOOG_RECORD_TYPE_KEYLINK, NULL, inception, expires);
	this->signingKey = NULL;
}

ZCert::~ZCert() {  
  delete this->object;
  delete this->sig;
  delete this->signingKey;
}


// Sign this certificate using the key provided
// signature = sign(RRSIG_RDATA | RR(1) | RR(2)... )
void ZCert::sign(ZPrivKey* key) {
	// Update the fields in the signature record
	this->sig->setKeyTag(key->getTag());
	this->sig->setSignerName(key->getOwner());	

	uint32_t bufferLen = this->sig->headerSize() + this->object->size();
	uint8_t* buffer = malloc_array(bufferLen);
	lite_assert(buffer);

	// Write everything about the signature (except the signature field itself)
	uint32_t headerLen = this->sig->serializeHead(buffer);

	// Write out the object to be signed
	this->object->serialize(buffer + headerLen);

	uint8_t* signature = malloc_array(ZKey::SIGNATURE_SIZE);
	lite_assert(signature);

	//printf("\nAbout to sign %d bytes\n", bufferLen);
	//dump_bytes(buffer, bufferLen);
	//printf("\n\n");

	key->sign(buffer, bufferLen, signature);

	this->sig->setSig(signature, ZKey::SIGNATURE_SIZE);

	this->signingKey = new ZPubKeyRecord(key->getPubKey()->getRecord());	

	//printf("Resulting signature is:\n");
	//dump_bytes(signature, ZKey::SIGNATURE_SIZE);
	//printf("\n\n");

	free_array(buffer);
	free_array(signature);
}

// Verify the validity of this certificate using the key provided
bool ZCert::verify(ZPubKey* key) {
	uint32_t bufferLen = this->sig->headerSize() + this->object->size();
	uint8_t* buffer = malloc_array(bufferLen);

	// Perform some basic checks first
	if (! (this->object && this->sig && key && key->getOwner()) ) {
		return false;
	}

	if (! (this->object->getType() == this->sig->getTypeCovered() &&		   
		   getCurrentTime() > this->sig->getInception() &&
		   getCurrentTime() < this->sig->getExpires() &&
		   this->sig->getSignerName() && key->getOwner() &&
		   *(this->sig->getSignerName()) == *(key->getOwner())) ) {
		return false;
	}


	// Write everything about the signature (except the signature field itself)
	uint32_t headerLen = this->sig->serializeHead(buffer);

	// Write out the object to be verified
	this->object->serialize(buffer + headerLen);

	//printf("\nAbout to verify %d bytes\n", bufferLen);
	//dump_bytes(buffer, bufferLen);
	//printf("\n\n");

	//printf("Incoming signature is:\n");
	//dump_bytes(this->sig->getSig(), ZKey::SIGNATURE_SIZE);
	//printf("\n\n");

	bool retval = key->verify(this->sig->getSig(), buffer, bufferLen);

	free_array(buffer);

	return retval;
}

// Verify the validity of this certificate using the cert chain provided, with rootkey as a trust anchor
// At each step, the label on the signed object must have the parent's label chain as a suffix;
// i.e., .com can sign example.com, which can sign www.example.com, but neither can sign harvard.edu
// Chain should be arranged in order of increasing specificity;
// i.e., trustAnchor signs key1 signs key2 signs key3 ... signs this cert
//bool ZCert::verifyTrustChain(ZCert** certs, uint32_t numCerts, ZPubKey* trustAnchor) {
//	lite_assert(certs);
//
//	ZPubKey* curKey = trustAnchor;
//	for (uint32_t i = 0; i < numCerts; i++) {
//		ZCert* curCert = certs[i];
//		lite_assert(curCert);
//
//		if (!curCert->verify(curKey)) {
//			return false;
//		}
//
//		ZPubKey* newKey = curCert->getPubKey();
//		if (! newKey || ! newKey->getOwner()->suffix(curKey->getOwner()) ) {
//			return false;
//		}
//
//		curKey = newKey;
//	}
//
//	// Chain checks out, so use the final key to verify this cert
//	return this->verify(curKey);
//}


// Retrieve the key signed by this certificate (not the signing key)
// Returns null if this is not a certificate for a key
ZPubKey* ZCert::getEndorsedKey() {
	if (!this->object ||
		!(this->object->getType() == ZRecord::ZOOG_RECORD_TYPE_KEY)) {
		return NULL;
	}

	return ((ZPubKeyRecord*)this->object)->getKey();
}

// Retrieve the public key corresponding to the private key that signed this certificate 
// Returns NULL if the certificate is unsigned
ZPubKey* ZCert::getEndorsingKey() {
	return this->signingKey->getKey();
}


// If this is a certificate for a key link, i.e., a cert that says KeyA speaksfor KeyB, 
// then getSpeakerKey returns KeyA and getSpokenKey returns KeyB
// Otherwise, returns NULL
ZPubKey* ZCert::getSpeakerKey() {
	if (!this->object ||
		!(this->object->getType() == ZRecord::ZOOG_RECORD_TYPE_KEYLINK)) {
		return NULL;
	}

	return ((ZKeyLinkRecord*)this->object)->getSpeaker();
}

// If this is a certificate for a key link, i.e., a cert that says KeyA speaksfor KeyB, 
// then getSpeakerKey returns KeyA and getSpokenKey returns KeyB
// Otherwise, returns NULL
ZPubKey* ZCert::getSpokenKey() {
	if (!this->object ||
		!(this->object->getType() == ZRecord::ZOOG_RECORD_TYPE_KEYLINK)) {
		return NULL;
	}

	return ((ZKeyLinkRecord*)this->object)->getSpokenFor();
}


// Retrieve information about the binary signed by this certificate 
// Returns null if this is not a certificate for a binary
ZBinaryRecord* ZCert::getEndorsedBinary() {
	if (!this->object ||
		!(this->object->getType() == ZRecord::ZOOG_RECORD_TYPE_BINARY)) {
		return NULL;
	}

	return (ZBinaryRecord*)this->object;
}

// Number of bytes needed to serialize this cert
uint32_t ZCert::size() const {
	uint32_t size = 0;
	
	size += this->object == NULL ? 4 : 4 + this->object->size();
	size += this->sig == NULL ? 4 : 4 + this->sig->size();
	size += this->signingKey == NULL ? 4 : 4 + this->signingKey->size();

	return size;
}

// Write the entire certificate out
// Format is: objectLength, object, sigLength, sig
void ZCert::serialize(uint8_t* buffer) {
	lite_assert(buffer);

	if (this->object == NULL) {
		WRITE_INT(buffer, 0);
	} else {
		WRITE_INT(buffer, this->object->size());
		this->object->serialize(buffer);
		buffer += this->object->size();
	}

	if (this->sig == NULL) {
		WRITE_INT(buffer, 0);
	} else {
		WRITE_INT(buffer, this->sig->size());
		this->sig->serialize(buffer);
		buffer += this->sig->size();
	}

	if (this->signingKey == NULL) {
		WRITE_INT(buffer, 0);
	} else {
		WRITE_INT(buffer, this->signingKey->size());
		this->signingKey->serialize(buffer);
		buffer += this->signingKey->size();
	}
}

bool operator==(ZCert &c1, ZCert &c2) {
	if ((c1.object == NULL && c2.object != NULL) ||
		(c1.object != NULL && c2.object == NULL)) {
		return false;
	}

	if (c1.object && !c1.object->isEqual(c2.object)) {
		return false;
	}

	if ((c1.sig == NULL && c2.sig != NULL) ||
		(c1.sig != NULL && c2.sig == NULL)) {
		return false;
	}
	
	if (c1.sig && !c1.sig->isEqual(c2.sig)) {
		return false;
	}

	if ((c1.signingKey == NULL && c2.signingKey != NULL) ||
		(c1.signingKey != NULL && c2.signingKey == NULL)) {
		return false;
	}
	
	if (c1.signingKey && !c1.signingKey->isEqual(c2.signingKey)) {
		return false;
	}

	return true;	    
}

bool operator!=(ZCert &c1, ZCert &c2) {
	return !(c1 == c2);
}
