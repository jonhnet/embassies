#include "Hashable.h"

uint32_t Hashable::_hash(const void *v_a)
{
	return ((Hashable*) v_a)->hash();
}

int Hashable::_cmp(const void *v_a, const void *v_b)
{
	return ((Hashable*) v_a)->cmp((Hashable*) v_b);
}
