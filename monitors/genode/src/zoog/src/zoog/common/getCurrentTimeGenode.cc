#include "pal_abi/pal_types.h"

uint32_t getCurrentTime()
{
	// crypto-time never changes.
	return 1357609200;
}
