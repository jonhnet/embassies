#include "RandomSeed.h"

RandomSeed::RandomSeed()
{
	for (uint32_t i=0; i<sizeof(_seed_bytes)/sizeof(_seed_bytes[0]); i++)
	{
		_seed_bytes[i] = 9;	// you never can tell
	}
}
