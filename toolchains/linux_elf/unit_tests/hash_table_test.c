#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "hash_table.h"
#include "standard_malloc_factory.h"

const int range = 1000;

uint32_t hash_int(const void *v_i)
{
	int *ip = (int*) v_i;
	return *(ip);
}

int cmp_int(const void *v_a, const void *v_b)
{
	int *ap = (int*) v_a;
	int *bp = (int*) v_b;

	return (*ap) - (*bp);
}

#if 0
void dump_bucket(HashTable *ht, int i)
{
	HashLink *hl = ht->buckets[i].next;
	for (; hl!=NULL; hl=hl->next)
	{
		fprintf(stderr, " %d,", ((int*)hl->datum)[0]);
	}
	fprintf(stderr, "--end\n");
}
#endif

int main()
{
	HashTable ht_alloc, *ht=&ht_alloc;
	bool members[range];
	int data[range];

	hash_table_init(ht, standard_malloc_factory_init(), hash_int, cmp_int);

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
			hash_table_remove(ht, &i);
			members[i] = false;
		}
		else
		{
//			fprintf(stderr, "insert %d\n", i);
			hash_table_insert(ht, &data[i]);
			members[i] = true;
		}
		//dump_bucket(ht, 2);
		if ((round&0xffff)==0)
		{
			fprintf(stderr, "round %d count %d\n", round, ht->count);
		}
		round++;
	}
	
	return 0;
}
