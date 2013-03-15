#include "c_qsort.h"
#include "LiteLib.h"

enum { CQSort_STATICALLY_ALLOCATED_SIZE_LIMIT = 16 };

typedef struct {
	uint8_t *base;
	size_t nmemb;
	size_t size;
	int(*compar)(const void *, const void *);
} CQSort ;

static void CQSort_ctor(CQSort* cthis, void *base, size_t nmemb, size_t size,
	int(*compar)(const void *, const void *));

static void CQSort_quicksort(CQSort* cthis, int left, int right);

static int CQSort_compare(CQSort* cthis, int ia, int ib);
static void CQSort_swap(CQSort* cthis, int ia, int ib);
static int CQSort_partition(CQSort* cthis, int left, int right, int pivotIndex);


void CQSort_ctor(CQSort *cthis, void *base, size_t nmemb, size_t size,
	int(*compar)(const void *, const void *))
{
	cthis->base = (uint8_t*) base;
	cthis->nmemb = nmemb;
	cthis->size = size;
	cthis->compar = compar;
}

int CQSort_compare(CQSort* cthis, int ia, int ib)
{
	return (cthis->compar)(
		cthis->base+cthis->size*ia, cthis->base+cthis->size*ib);
}

void CQSort_swap(CQSort* cthis, int ia, int ib)
{
	// I now use this in a context (Genode) where alloca isn't
	// available. Plus alloca is evil. So let's do something differently-evil.
	// (could demand an allocator, and allocate this at QSort ctor time.)
	//void *space = alloca(size);
	char space[CQSort_STATICALLY_ALLOCATED_SIZE_LIMIT];

	// specialization sure would make this go faster.
	lite_memcpy(space, cthis->base+cthis->size*ia, cthis->size);
	lite_memcpy(cthis->base+cthis->size*ia, cthis->base+cthis->size*ib, cthis->size);
	lite_memcpy(cthis->base+cthis->size*ib, space, cthis->size);
}

// Implementation of in-place quicksort using terminology
// to match http://en.wikipedia.org/wiki/Quicksort
int CQSort_partition(CQSort* cthis, int left, int right, int pivotIndex)
{
	CQSort_swap(cthis, pivotIndex, right);
	int storeIndex = left;
	int i;
	for (i=left; i<right; i++)
	{
		if (CQSort_compare(cthis, i, right)<0)
		{
			CQSort_swap(cthis, i, storeIndex);
			storeIndex+=1;
		}
	}
	CQSort_swap(cthis, storeIndex, right);
	return storeIndex;
}

void CQSort_quicksort(CQSort* cthis, int left, int right)
{
	if (left>=right)
	{
		return;	// sorted! yay
	}
	int pivotIndex = (left+right)/2;
	lite_assert(left<=pivotIndex && pivotIndex<=right);
	int pivotNewIndex = CQSort_partition(cthis, left, right, pivotIndex);
	CQSort_quicksort(cthis, left, pivotNewIndex-1);
	CQSort_quicksort(cthis, pivotNewIndex+1, right);
}

void c_qsort(void *base, size_t nmemb, size_t size,
	int(*compar)(const void *, const void *))
{
	lite_assert(size < CQSort_STATICALLY_ALLOCATED_SIZE_LIMIT);
	CQSort qsort;
	CQSort_ctor(&qsort, base, nmemb, size, compar);
	CQSort_quicksort(&qsort, 0, nmemb-1);
}
