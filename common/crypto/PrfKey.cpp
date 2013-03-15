#include "LiteLib.h"
#include "PrfKey.h"

PrfKey::PrfKey(RandomSupply* rand) : ZSymKey(rand) {
	rijndaelKeySetupEnc(roundKeys, this->getBytes(), this->getNumBits());
}

PrfKey::PrfKey(uint8_t* buffer, uint32_t len) : ZSymKey(buffer, len) {
	rijndaelKeySetupEnc(roundKeys, this->getBytes(), this->getNumBits());
}

void PrfKey::evaluatePrf(uint8_t input[BLOCK_SIZE], uint8_t output[BLOCK_SIZE]) {
	rijndaelBlockEncrypt(roundKeys, this->getNumAesRounds(), input, output);
}

uint8_t PrfKey::getNumAesRounds() {
	switch (this->numBits) {
		case 128:
			return 10;
		case 192:
			return 12;
		case 256:
			return 14;
	}

	lite_assert(false);
	return 0;
}
