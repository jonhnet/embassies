#include "LiteLib.h"
#include "ZPrivKey.h"
#include "CryptoException.h"

// Create a key from an existing context
ZPrivKey::ZPrivKey(rsa_context* rsa_ctx, const DomainName* owner, ZPubKey* pubKey)
	: ZKey(rsa_ctx, owner) {
	this->pubKey = new ZPubKey(pubKey);
}

ZPrivKey::~ZPrivKey() {
	// General key stuff
	delete this->owner;
	rsa_free(this->rsa_ctx);

	// PrivKey-specific stuff
	delete this->pubKey;
}


void ZPrivKey::sign(uint8_t* buffer, uint32_t length, uint8_t* signature) {
	hash_t* digest = new hash_t; 

	zhash(buffer, length, digest);

	lite_assert(!rsa_pkcs1_sign(this->rsa_ctx, RSA_PRIVATE, PKCS_SIG_ALG_ID, HASH_SIZE, digest->bytes, signature));
	delete digest;
}

// Bytes needed to serialize this key
uint32_t ZPrivKey::size() {
	return this->keySize() + this->pubKey->size();
}

uint32_t ZPrivKey::serialize(uint8_t* buffer) {
	uint32_t keySize = serializeKey(buffer);
	buffer += keySize;

	this->pubKey->serialize(buffer);
	buffer += this->pubKey->size();

	return keySize + this->pubKey->size();
}

// Create a key from one we previously serialized
ZPrivKey::ZPrivKey(uint8_t* serializedKey, uint32_t size) 
	: ZKey(serializedKey, size) {
	serializedKey += this->keySize();
	
	this->pubKey = new ZPubKey(serializedKey, size);
}
