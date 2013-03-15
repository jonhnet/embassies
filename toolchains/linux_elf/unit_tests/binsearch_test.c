#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "LiteLib.h"
#include "binsearch.h"

const int range = 10;

int cmp_int(const void *v_a, const void *v_b)
{
	int *ap = (int*) v_a;
	int *bp = (int*) v_b;

	return (*ap) - (*bp);
}

int main()
{
	int data[range];

	int i;
	for (i=0; i<range; i++)
	{
		data[i] = i;
	}

	int round = 0;
	while (1)
	{
		int i = ((uint32_t) random()) % range;

		void *key_ptr = &data[i];

		lite_assert(((int*)key_ptr)[0] == i);

		void *ptr = binsearch(key_ptr, data, range, sizeof(data[0]), cmp_int);

		lite_assert(((int*)ptr)[0] == i);

		if ((round&0xffff)==0)
		{
			fprintf(stderr, "round %d\n", round);
		}
		round++;
	}
	
	return 0;
}
