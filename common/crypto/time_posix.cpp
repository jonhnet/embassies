#include <time.h>

#include "pal_abi/pal_types.h"

uint32_t getCurrentTime()
{
	return time(NULL);
}
