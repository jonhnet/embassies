#include "array_alloc.h"
#include "LiteLib.h"
#include "ZPubKey.h"
#include "ZSigRecord.h"
#include "CryptoException.h"

// Create a key from an existing RSA context
ZPubKey::ZPubKey(rsa_context* rsa_ctx, const DomainName* owner) 
	: ZKey(rsa_ctx, owner) {
	uint32_t keyLen = this->keyRecordSize();
	uint8_t* bytes = malloc_array(keyLen);
	lite_assert(bytes);
	this->getKeyRecordBytes(bytes);
	this->record = new ZPubKeyRecord(owner, bytes, keyLen);
	free_array(bytes);
}

// Duplicate an existing key
ZPubKey::ZPubKey(ZPubKey* key) {
	lite_assert(key);

	// Copy the ZKey info by writing key's data to a buffer and reading it back out
	uint8_t* buffer = malloc_array(key->size());
	lite_assert(buffer);
	key->serializeKey(buffer);  
	this->parseKey(buffer, key->size());
	free_array(buffer);

	this->record = new ZPubKeyRecord(key->record);
	lite_assert(this->record);
}

// Create a new key from the key bytes in a DNSKEY RR
ZPubKey::ZPubKey(uint16_t algorithm, uint8_t* keyBytes, uint32_t keySize, DomainName* owner) {
	lite_assert(algorithm == SIG_ALG_ID);
	uint8_t* originalKeyBytes = keyBytes;

	this->owner = new DomainName(owner);
	lite_assert(this->owner);

	this->rsa_ctx = new rsa_context;
	lite_assert(this->rsa_ctx);
	rsa_init(this->rsa_ctx, RSA_PKCS_V15, SIG_ALG_ID);
	this->rsa_ctx->ver = 0;
	
	// Copy over the RSA info
	uint16_t exponentBytes = 0;

	PARSE_BYTE(keyBytes, exponentBytes);
	if (exponentBytes == 0) {
		PARSE_SHORT(keyBytes, exponentBytes);
	}

	mpi_read_binary(&this->rsa_ctx->E, keyBytes, exponentBytes);
	keyBytes += exponentBytes;	

	mpi_read_binary(&this->rsa_ctx->N, keyBytes, keySize - (keyBytes - originalKeyBytes));
	this->rsa_ctx->len = ( mpi_msb( &this->rsa_ctx->N ) + 7 ) >> 3;
	
	this->record = new ZPubKeyRecord(owner, originalKeyBytes, keySize);
	lite_assert(this->record);
}

ZPubKey::~ZPubKey() {
	// General key stuff
	delete this->owner;
	rsa_free(this->rsa_ctx);

	// PubKey-specific stuff
	delete this->record;
}

uint16_t ZPubKey::getTag() {
	return this->record ? this->record->getTag() : 0;
}

bool ZPubKey::verify(uint8_t* signature, uint8_t* buffer, uint32_t length) {
	hash_t* digest = new hash_t;

	zhash(buffer, length, digest);

	bool result = 0 == rsa_pkcs1_verify(this->rsa_ctx, RSA_PUBLIC, PKCS_SIG_ALG_ID, HASH_SIZE, digest->bytes, signature);
	delete digest;

	return result;
}

// Return number of bytes needed for this key's actual key material
uint32_t ZPubKey::keyRecordSize() {
	uint32_t exponentBytes = mpi_write_binary(&this->rsa_ctx->E, NULL, 0);
	uint32_t modBytes = mpi_write_binary(&this->rsa_ctx->N, NULL, 0);
	uint32_t bytesNeeded = exponentBytes + modBytes;

	if (exponentBytes <= 255) {
		bytesNeeded += 1; // One byte to describe exponent length
	} else {
		bytesNeeded += 3; // One to signal long exponent, 2 for length
	}
	
	return bytesNeeded;
}

// Return actual key bytes for use in a pubkey record
// See RFC 3110, Section 2: http://tools.ietf.org/html/rfc3110
void ZPubKey::getKeyRecordBytes(uint8_t* buffer) {

	uint32_t exponentBytes = mpi_write_binary(&this->rsa_ctx->E, buffer, 0);

	if (exponentBytes <= 255) {
		WRITE_BYTE(buffer, exponentBytes);
	} else {
		WRITE_BYTE(buffer, 0);
		WRITE_SHORT(buffer, exponentBytes);
	}

	// Write the exponent
	mpi_write_binary(&this->rsa_ctx->E, buffer, exponentBytes);
	buffer += exponentBytes;

	// Write the modulus
	uint32_t modBytes = mpi_write_binary(&this->rsa_ctx->N, buffer, 0);
	mpi_write_binary(&this->rsa_ctx->N, buffer, modBytes);
	buffer += modBytes;
}


// Bytes needed to serialize this key
uint32_t ZPubKey::size() {
	return this->keySize() + this->record->size();
}

uint32_t ZPubKey::serialize(uint8_t* buffer) {
	uint32_t keySize = serializeKey(buffer);
	buffer += keySize;

	this->record->serialize(buffer);
	buffer += this->record->size();

	return keySize + this->record->size();
}

// Create a key from one we previously serialized
ZPubKey::ZPubKey(uint8_t* serializedKey, uint32_t size) 
	: ZKey(serializedKey, size) {
	serializedKey += this->keySize();

	this->record = new ZPubKeyRecord(serializedKey, size - this->keySize());
}


// Verify the validity of the label on this key using the cert chain provided, with trustAnchor as a root of trust
// At each step, the label on the signed object must have the parent's label chain as a suffix;
// i.e., .com can sign example.com, which can sign www.example.com, but neither can sign harvard.edu
// Chain should be arranged in order of increasing specificity;
// i.e., trustAnchor signs key1 signs key2 signs key3 ... signs this key
bool ZPubKey::verifyLabel(ZCertChain* chain, ZPubKey* trustAnchor) {
	lite_assert(chain);

	ZPubKey* curKey = trustAnchor;
	for (uint32_t i = 0; i < chain->getNumCerts(); i++) {
		ZCert* curCert = (*chain)[i];
		lite_assert(curCert);

		if (!curCert->verify(curKey)) {
			return false;
		}

		ZPubKey* newKey = curCert->getEndorsedKey();
		if (! newKey || !newKey->getOwner()->suffix(curKey->getOwner())) { 
			return false;
		}

		if (this->isEqual(newKey)) {
			return true;
		}

		curKey = newKey;
	}

	// The cert chain didn't actually contain our key!
	return false;
}

bool ZPubKey::isEqual(ZPubKey* key) {
	if (key == NULL) { return false; }

	// The keys are the same if their serialized form is the same
	return this->getRecord()->isEqual(key->getRecord());
}
