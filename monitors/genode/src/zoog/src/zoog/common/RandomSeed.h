#pragma once

#include "RandomSupply.h"

// TODO Non-random stand-in for a random seed.
class RandomSeed
{
private:
	uint8_t _seed_bytes[RandomSupply::SEED_SIZE];

public:
	RandomSeed();
	uint8_t* get_seed_bytes() { return _seed_bytes; }
};
