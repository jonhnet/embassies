#include "lite_log2.h"


// NB log2(0) == 0
// [but current customer, cheesymalloc, never asks for a log of a value
// smaller than the overhead, anyway.]
int lite_log2(uint32_t len)
{
	uint32_t val = len;
	uint32_t logval = 0;

	int bits;
	for (bits=4; bits>=0; bits--)
	{
		unsigned int scale = 1<<bits;	// 16,8,4,2,1
		if (val >= (uint32_t)(1<<scale))
		{
			val >>= scale;
			logval += scale;
		}
	}
	return logval+1;
}

