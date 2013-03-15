#include "SymEncKey.h"
#include "PrfKey.h"
#include "KeyDerivationKey.h"
#include "LiteLib.h"
#include "array_alloc.h"

// Somewhat convoluted: Use the incoming bytes to create 
// a PRF key for encrypting and a MAC key for auth'ing the encryption
SymEncKey::SymEncKey(uint8_t* buffer, uint32_t len) : PrfKey(buffer, len) {
	KeyDerivationKey* kdf = new KeyDerivationKey(buffer, len);

	uint8_t encLabel[] = "SymEncKey-Secrecy";
	PrfKey* tmpPrfKey = kdf->derivePrfKey(encLabel, sizeof(encLabel));
	lite_assert(tmpPrfKey);
	lite_memcpy(this->bytes, tmpPrfKey->getBytes(), tmpPrfKey->getNumBytes());
	delete tmpPrfKey;

	uint8_t macLabel[] = "SymEncKey-Integrity";
	this->macKey = kdf->deriveMacKey(macLabel, sizeof(macLabel));
	lite_assert(this->macKey);

	delete kdf;
}

SymEncKey::~SymEncKey() {
	delete this->macKey;
}

// Encrypts the buffer and returns a len+buffer of ciphertext 
Ciphertext* SymEncKey::encrypt(RandomSupply* rand, uint8_t* buffer, uint32_t len) { 
	Ciphertext* ret = (Ciphertext*) malloc_array(sizeof(Ciphertext) + len);
	lite_assert(ret);
	ret->len = len;

	// Choose a random nonce 
	rand->get_random_bytes(ret->IV, BLOCK_CIPHER_SIZE/8);

	// Do the encryption
	CtrMode(buffer, len, ret->IV, ret->bytes);

	// Integrity protection needs to cover IV plus the ciphertext
	Mac* mac = this->macKey->mac(rand, ret->IV, BLOCK_CIPHER_SIZE/8 + len);
	ret->mac = *mac;

	return ret;	
}


// Expects a len+buffer of ciphertext and returns a len+buffer of plaintext
// Returns NULL if decryption fails
Plaintext* SymEncKey::decrypt(Ciphertext* cipher) {
	// TODO: Ugly expedient to make sure MAC is computed over data
	//       that's aligned the same as it was initially,
	//       i.e., cipher->mac.prefixOverhang before a 16-byte boundary
	uint32_t data_size = BLOCK_CIPHER_SIZE/8 + cipher->len;
	uint8_t* buffer = (uint8_t*) malloc_array(32 + data_size);
	uint32_t offset = 16 - (((uint32_t) buffer) % 16); // Dist to a 16-bit boundary
	offset += 16 - (cipher->mac.prefixOverhang % 16);	// Same alignment as before
	lite_memcpy(buffer+offset, cipher->IV, data_size);

	bool validMac = macKey->verify(&cipher->mac, buffer+offset, data_size);
	free_array(buffer);
	if (!validMac) {
		return NULL;
	}

	Plaintext* ret = (Plaintext*) malloc_array(sizeof(Plaintext) + cipher->len);
	lite_assert(ret);
	ret->len = cipher->len;
	
	CtrMode(cipher->bytes, cipher->len, cipher->IV, ret->bytes);

	return ret;
}

void SymEncKey::CtrMode(uint8_t* input, uint32_t len, uint8_t IV[BLOCK_CIPHER_SIZE/8], uint8_t* output) {
	const uint32_t BYTES_IN_BLOCK = BLOCK_CIPHER_SIZE/8;
	uint8_t counterBytes[BYTES_IN_BLOCK];
	uint8_t randBytes[BYTES_IN_BLOCK];

	// Make a local copy, so the IV we send out isn't disturbed
	lite_memcpy(counterBytes, IV, BYTES_IN_BLOCK);
	uint64_t* counter = (uint64_t*)counterBytes;	// Use lower 64-bits 

	// Work in block-sized chunks
	for (uint32_t i = 0; i < (len - 1) / BYTES_IN_BLOCK + 1; i++, (*counter)++) {
		this->evaluatePrf(counterBytes, randBytes);
		
		// TODO: Convert to do 32-bit chunks at a time
		for (uint32_t j = 0; 
		     j < BYTES_IN_BLOCK && 
				 j + i * BYTES_IN_BLOCK < len; 
				 j++) {
			uint32_t index = j + i * BYTES_IN_BLOCK;
			output[index] = input[index] ^ randBytes[j];
		}
	}
}
