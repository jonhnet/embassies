//#include <alloca.h>
#include "zoog_qsort.h"
#include "LiteLib.h"

class QSort {
public:
	QSort(void *base, size_t nmemb, size_t size,
		int(*compar)(const void *, const void *));

	void quicksort(int left, int right);
	enum { STATICALLY_ALLOCATED_SIZE_LIMIT = 16 };

private:
	int compare(int ia, int ib);
	void swap(int ia, int ib);
	int partition(int left, int right, int pivotIndex);

	uint8_t *base;
	size_t nmemb;
	size_t size;
	int(*compar)(const void *, const void *);
};

QSort::QSort(void *base, size_t nmemb, size_t size,
	int(*compar)(const void *, const void *))
{
	this->base = (uint8_t*) base;
	this->nmemb = nmemb;
	this->size = size;
	this->compar = compar;
}

int QSort::compare(int ia, int ib)
{
	return (compar)(base+size*ia, base+size*ib);
}

void QSort::swap(int ia, int ib)
{
	// I now use this in a context (Genode) where alloca isn't
	// available. Plus alloca is evil. So let's do something differently-evil.
	// (could demand an allocator, and allocate this at QSort ctor time.)
	//void *space = alloca(size);
	char space[STATICALLY_ALLOCATED_SIZE_LIMIT];

	// specialization sure would make this go faster.
	lite_memcpy(space, base+size*ia, size);
	lite_memcpy(base+size*ia, base+size*ib, size);
	lite_memcpy(base+size*ib, space, size);
}

// Implementation of in-place quicksort using terminology
// to match http://en.wikipedia.org/wiki/Quicksort
int QSort::partition(int left, int right, int pivotIndex)
{
	swap(pivotIndex, right);
	int storeIndex = left;
	for (int i=left; i<right; i++)
	{
		if (compare(i, right)<0)
		{
			swap(i, storeIndex);
			storeIndex+=1;
		}
	}
	swap(storeIndex, right);
	return storeIndex;
}

void QSort::quicksort(int left, int right)
{
	if (left>=right)
	{
		return;	// sorted! yay
	}
	int pivotIndex = (left+right)/2;
	lite_assert(left<=pivotIndex && pivotIndex<=right);
	int pivotNewIndex = partition(left, right, pivotIndex);
	quicksort(left, pivotNewIndex-1);
	quicksort(pivotNewIndex+1, right);
}

void zoog_qsort(void *base, size_t nmemb, size_t size,
	int(*compar)(const void *, const void *))
{
	lite_assert(size < QSort::STATICALLY_ALLOCATED_SIZE_LIMIT);
	QSort qsort(base, nmemb, size, compar);
	qsort.quicksort(0, nmemb-1);
}
