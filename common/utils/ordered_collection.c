#include "LiteLib.h"
#include "ordered_collection.h"

void ordered_collection_init(OrderedCollection *oc, MallocFactory *mf, CmpFunc *cmp_func)
{
	oc->mf = mf;
	oc->cmp_func = cmp_func;

	oc->capacity = 10;
	oc->data = mf_malloc(mf, sizeof(void*)*oc->capacity);
	oc->count = 0;
	cheesy_lock_init(&oc->mutex);
}

void _ordered_collection_grow_lockheld(OrderedCollection *oc)
{
	int new_capacity = oc->capacity * 2;
	void *new_data = mf_realloc(oc->mf, oc->data,
		sizeof(void*)*oc->capacity, sizeof(void*)*new_capacity);
	oc->capacity = new_capacity;
	oc->data = new_data;
}

int _ordered_collection_find_ge_lockheld(OrderedCollection *oc, void *key)
{
	// sweet, sweet linear search
	int i;
	for (i=0; i<oc->count; i++)
	{
		if ((oc->cmp_func)(oc->data[i], key) >= 0)
		{
			break;
		}
	}
	return i;
}

void ordered_collection_insert(OrderedCollection *oc, void *datum)
{
	cheesy_lock_acquire(&oc->mutex);
	if (oc->count == oc->capacity)
	{
		_ordered_collection_grow_lockheld(oc);
	}
	lite_assert(oc->count < oc->capacity);

	if (oc->count==0)
	{
		oc->data[0] = datum;
		oc->count += 1;
		goto exit;
	}

	int i = _ordered_collection_find_ge_lockheld(oc, datum);

	lite_assert(i==oc->count || (oc->cmp_func)(datum, oc->data[i]) !=0 );

	// mmmmm linear insert nom nom
	int j;
	for (j=oc->count; j>i; j--)
	{
		oc->data[j] = oc->data[j-1];
	}
	oc->data[i] = datum;
	oc->count += 1;

exit:
	cheesy_lock_release(&oc->mutex);
}

void *ordered_collection_lookup_ge(OrderedCollection *oc, void *key)
{
	void *result;

	cheesy_lock_acquire(&oc->mutex);
	int i = _ordered_collection_find_ge_lockheld(oc, key);
	lite_assert(i<=oc->count);
	if (i==oc->count)
	{
		result = NULL;
		goto exit;
	}
	result = oc->data[i];

exit:
	cheesy_lock_release(&oc->mutex);
	return result;
}

int _ordered_collection_find_le_lockheld(OrderedCollection *oc, void *key)
	// NB returns -1 on failure! (ge returns oc->count on failure, which,
	// under appropriate preconditions is a valid oc->data slot.)
{
	// sweet, sweet linear search
	int i;
	for (i=oc->count-1; i>=0; i--)
	{
		if ((oc->cmp_func)(oc->data[i], key) <= 0)
		{
			break;
		}
	}
	return i;
}

void *ordered_collection_lookup_le(OrderedCollection *oc, void *key)
{
	void *result;

	cheesy_lock_acquire(&oc->mutex);
	int i = _ordered_collection_find_le_lockheld(oc, key);
	if (i<0)
	{
		result = NULL;
		goto exit;
	}
	lite_assert(i<oc->count);
	result = oc->data[i];

exit:
	cheesy_lock_release(&oc->mutex);
	return result;
}

void *ordered_collection_lookup_eq(OrderedCollection *oc, void *key)
{
	void *datum = ordered_collection_lookup_ge(oc, key);
	if ((oc->cmp_func)(datum, key) != 0)
	{
		datum = NULL;
	}
	return datum;
}

void ordered_collection_remove(OrderedCollection *oc, void *key)
{
	cheesy_lock_acquire(&oc->mutex);

	int i = _ordered_collection_find_ge_lockheld(oc, key);
	lite_assert(i<oc->count);	// whoah, key > every datum we've got
	lite_assert((oc->cmp_func)(key, oc->data[i]) ==0 );	// expected key to match something exactly

	// radical linear awesomeness
	int j;
	for (j=i; j<oc->count-1; j++)
	{
		oc->data[j] = oc->data[j+1];
	}
	oc->count -= 1;

	cheesy_lock_release(&oc->mutex);
}
