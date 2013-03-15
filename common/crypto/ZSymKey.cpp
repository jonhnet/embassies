#include "array_alloc.h"
#include "LiteLib.h"
#include "ZSymKey.h"
#include "CryptoException.h"
#include "math_util.h"

ZSymKey::ZSymKey(RandomSupply* random_supply, uint32_t numBits) {
	lite_assert(numBits % 8 == 0);  // Keeps life simple
	lite_assert(numBits / 8 < 100);  // Keeps life simple

	uint8_t tmp[100];	// TODO: Bleh, this is silly
	random_supply->get_random_bytes(tmp, sizeof(tmp));

	init(tmp, numBits);
}
	
ZSymKey::ZSymKey(uint8_t* buffer, uint32_t len) {
	init(buffer, len*8);
}

void ZSymKey::init(uint8_t* buffer, uint32_t numBits) {
	if (!(numBits == 128 || numBits == 192 || numBits == 256)) {
		Throw(CryptoException::BAD_INPUT, "Invalid AES key size");
	}

	this->bytes = malloc_array(numBits / 8);
	this->numBits = numBits;
	
	lite_memcpy(this->bytes, buffer, numBits / 8);
}

ZSymKey::~ZSymKey() {
	free_array(this->bytes);
}
	
// Requires bufferLen >= size()
void ZSymKey::serialize(uint8_t* buffer, uint32_t bufferLen) {
	lite_assert(bufferLen >= this->size());
	lite_memcpy(buffer, this->getBytes(), this->getNumBytes());
}

uint32_t ZSymKey::size() {
	return this->getNumBytes();
}


uint8_t* ZSymKey::getBytes() {
	return this->bytes;
}

uint32_t ZSymKey::getNumBytes() {
	return this->numBits / 8;
}

uint32_t ZSymKey::getNumBits() {
	return this->numBits;
}

