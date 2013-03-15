#include "LiteLib.h"
#include "ZKeyPair.h"
#include "CryptoException.h"

// Generates an anonymous keypair
ZKeyPair::ZKeyPair(RandomSupply* random_supply) {
	DomainName owner("ZOOG", sizeof("ZOOG"));
	this->rsa_ctx = new rsa_context;
	lite_assert(this->rsa_ctx);
	rsa_init(this->rsa_ctx, RSA_PKCS_V15, RSA_SHA512);
	lite_assert(!rsa_gen_key(this->rsa_ctx, PUB_KEY_SIZE, 65537, random_supply));

	this->pub = new ZPubKey(this->rsa_ctx, &owner);
	this->priv = new ZPrivKey(this->rsa_ctx, &owner, this->pub);
}

ZKeyPair::ZKeyPair(RandomSupply* random_supply, const DomainName* owner) {
	this->rsa_ctx = new rsa_context;
	lite_assert(this->rsa_ctx);
	rsa_init(this->rsa_ctx, RSA_PKCS_V15, RSA_SHA512);
	lite_assert(!rsa_gen_key(this->rsa_ctx, PUB_KEY_SIZE, 65537, random_supply));

	this->pub = new ZPubKey(this->rsa_ctx, owner);
	this->priv = new ZPrivKey(this->rsa_ctx, owner, this->pub);	
}

ZKeyPair::~ZKeyPair() {
	delete this->priv;
	delete this->pub;
	delete this->rsa_ctx;
}


// Bytes needed to serialize this keypair
uint32_t ZKeyPair::size() {
	return this->priv->size() + this->pub->size();
}

uint32_t ZKeyPair::serialize(uint8_t* buffer) {
	this->priv->serialize(buffer);
	buffer += this->priv->size();

	this->pub->serialize(buffer);
	buffer += this->pub->size();

	return this->size();
}

// Create a keypair from one we previously serialized
ZKeyPair::ZKeyPair(uint8_t* serializedKeyPair, uint32_t size) {
	this->priv = new ZPrivKey(serializedKeyPair, size);
	serializedKeyPair += this->priv->size();

	this->pub = new ZPubKey(serializedKeyPair, size - this->priv->size());
	serializedKeyPair += this->pub->size();

	this->rsa_ctx = this->pub->getContext();
}
