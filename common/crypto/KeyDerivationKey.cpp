#include "KeyDerivationKey.h"
#include "array_alloc.h"

MacKey* KeyDerivationKey::deriveMacKey(uint8_t* tag, uint32_t tagLen) {
	//lite_assert(MacKey::SIZE == PrfKey::BLOCK_SIZE);
	uint8_t buffer[PrfKey::BLOCK_SIZE];
	deriveKey(buffer, tagHash(tag, tagLen));
	return new MacKey(buffer, sizeof(buffer));
}

SymEncKey* KeyDerivationKey::deriveEncKey(uint8_t* tag, uint32_t tagLen) {
	//lite_assert(EncKey::SIZE == PrfKey::BLOCK_SIZE);
	uint8_t buffer[PrfKey::BLOCK_SIZE];
	deriveKey(buffer, tagHash(tag, tagLen));
	return new SymEncKey(buffer, sizeof(buffer));
}


KeyDerivationKey* KeyDerivationKey::deriveKeyDerivationKey(uint8_t* tag, uint32_t tagLen) {
	//lite_assert(KeyDerivationKey::SIZE == PrfKey::BLOCK_SIZE);
	uint8_t buffer[PrfKey::BLOCK_SIZE];
	deriveKey(buffer, tagHash(tag, tagLen));
	return new KeyDerivationKey(buffer, sizeof(buffer));
}

KeyDerivationKey* KeyDerivationKey::deriveKeyDerivationKey(ZPubKey* pubkey) {
	uint8_t* keyBytes =  malloc_array(pubkey->size());
  pubkey->serialize(keyBytes);
	KeyDerivationKey* ret = deriveKeyDerivationKey(keyBytes, pubkey->size());
	free_array(keyBytes);

	return ret;
}

PrfKey* KeyDerivationKey::derivePrfKey(uint8_t* tag, uint32_t tagLen) {
	uint8_t buffer[PrfKey::BLOCK_SIZE];
	deriveKey(buffer, tagHash(tag, tagLen));
	return new PrfKey(buffer, sizeof(buffer));
}

hash_t KeyDerivationKey::tagHash(uint8_t* tag, uint32_t tagLen) {
	hash_t hash;
	zhash(tag, tagLen, &hash);
	return hash;
}

void KeyDerivationKey::deriveKey(uint8_t buffer[PrfKey::BLOCK_SIZE], hash_t tagHash) {
	lite_assert(sizeof(tagHash) >= PrfKey::BLOCK_SIZE);
	this->evaluatePrf((uint8_t*)&tagHash, buffer);
}

	




















