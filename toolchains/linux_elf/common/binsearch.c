#include <stdio.h> // NULL
#include "binsearch.h"

void *binsearch(const void *key, const void *base,
	uint32_t nmemb, uint32_t size,
	int (*compar)(const void *, const void *))
{
	if (nmemb==0)
	{
		return NULL;
	}

	int lo = 0;
	int hi =nmemb;

	while (lo<hi)
	{
		int mid = (lo+hi)/2;
		const void *mid_ptr = base+size*mid;
		int v = (compar)(key, mid_ptr);
		if (v==0)
		{
			return (void*) mid_ptr;
		}
		else if (v<0)
		{
			hi = mid;
		}
		else // v>0
		{
			lo = mid+1;
		}
	}
	return NULL; // not reached
}
