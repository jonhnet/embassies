#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "LiteLib.h"
#include "ordered_collection.h"
#include "standard_malloc_factory.h"

const int range = 1000;

int cmp_int(const void *v_a, const void *v_b)
{
	int *ap = (int*) v_a;
	int *bp = (int*) v_b;

	return (*ap) - (*bp);
}

void dump_oc(OrderedCollection *oc)
{
	int i;
	for (i=0; i<oc->count; i++)
	{
		int *datum = (int*) oc->data[i];
		fprintf(stderr, "%d, ", *datum);
	}
	fprintf(stderr, "\n");
}

int main()
{
	OrderedCollection oc_alloc, *oc=&oc_alloc;
	bool members[range];
	int data[range];

	ordered_collection_init(oc, standard_malloc_factory_init(), cmp_int);

	memset(members, 0, sizeof(members));
	int i;
	for (i=0; i<range; i++)
	{
		data[i] = i;
	}

	int round = 0;
	while (1)
	{
		int i = ((uint32_t) random()) % range;
		if (members[i])
		{
//			fprintf(stderr, "removing %d\n", i);
			ordered_collection_remove(oc, &i);
			members[i] = false;
		}
		else
		{
//			fprintf(stderr, "insert %d\n", i);
			ordered_collection_insert(oc, &data[i]);
			members[i] = true;
		}

		// sanity-check lookup_le.
		// (lookup_ge is partially tested by insert).
		if (i>0)
		{
			void *datum = ordered_collection_lookup_le(oc, &data[i-1]);
			int low;
			if (datum==NULL)
			{
				low = 0;
			}
			else
			{
				low = ((int*) datum)[0]+1;
			}
			int j;
			for (j=low; j<i; j++)
			{
				lite_assert(!members[j]);
			}
		}
//		dump_oc(oc);
		if ((round&0xffff)==0)
		{
			fprintf(stderr, "round %d count %d\n", round, oc->count);
		}
		round++;
	}
	
	return 0;
}
