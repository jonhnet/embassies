#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>

#include "LiteLib.h"
#include "cheesy_snprintf.h"
#include "zoog_qsort.h"

int __qsort_compar__(const void *v_a, const void *v_b)
{
	int *a = (int*) v_a;
	int *b = (int*) v_b;
	return a[0]-b[0];
}

void test(int count, int range)
{
	int space = count*sizeof(int);
	int *array = alloca(space);
	int i;
	for (i=0; i<count; i++)
	{
		array[i] = random() % range;
	}

	int *refarray = alloca(space);
	lite_memcpy(refarray, array, space);
	qsort(refarray, count, sizeof(int), __qsort_compar__);
	zoog_qsort(array, count, sizeof(int), __qsort_compar__);

	if (lite_memcmp(refarray, array, space)!=0)
	{
		fprintf(stderr, "test fails.\n");
		lite_assert(false);
	}

	for (i=0; i<count; i++)
	{
		printf("%d ", array[i]);
	}
	printf("\n");
}

int main()
{
	int count;
	for (count = 0; count < 200; count += 7)
	{
		test(count, 5);
		test(count, count);
		test(count, count*10);
	}
	return 0;
}
