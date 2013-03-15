#include "LiteLib.h"
#include "RandomSupply.h"

//void RandomSupply::inject_entropy(uint8_t *bytes, uint32_t length)
//{
//	entropy_received_bytes += length;
//	for (uint32_t i=0; i<length; i++)
//	{
//		random_state = (random_state*131) + bytes[i];
//	}
//}

RandomSupply::RandomSupply(uint8_t bytes[SEED_SIZE]) {
	lite_memset(counter, 0, sizeof(counter));
	rijndaelKeySetupEnc(round_keys, bytes, SYM_KEY_BITS);
	pool_ptr = sizeof(pool);	// Make sure we invoke PRF on first rand request
}

static int getNumAesRounds(int num_bits) {
  switch (num_bits) {
    case 128:
      return 10;
    case 192:
      return 12;
    case 256:
      return 14;
		default:
			lite_assert(0);
			return 0;
  }
}

uint8_t RandomSupply::get_random_byte() {
	if (pool_ptr >=  sizeof(pool)) {
		// Increment the counter (32 bits at a time)
		bool all_overflow = true;
		for (uint32_t i = 0; i < sizeof(counter)/sizeof(uint32_t); i++) {
			((uint32_t*)counter)[i]++;
			if (((uint32_t*)counter)[i] == 0) { 
				// We overflowed, so continue to next position
			} else { // No overflow.  We're done;
				all_overflow = false;
				break;
			}
		}
		lite_assert(!all_overflow);	// Otherwise, we're reusing counter values

		// Apply the PRF
		rijndaelBlockEncrypt(round_keys, getNumAesRounds(SYM_KEY_BITS), counter, pool);

		// Reset the pointer
		pool_ptr = 0;
	}

	return pool[pool_ptr++];
}

void RandomSupply::get_random_bytes(uint8_t *out_bytes, uint32_t length)
{
	for (uint32_t i=0; i<length; i++) {
		out_bytes[i] = get_random_byte();
	}
}
