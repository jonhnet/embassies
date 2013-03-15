#include "HashableHash.h"
#include "hash_table.h"

HashableHash::HashableHash(hash_t *hash, uint32_t found_at_offset)
{
	this->_hash = *hash;
	this->_found_at_offset = found_at_offset;
	this->_n_occurrences = 1;
}

uint32_t HashableHash::hash()
{
	return hash_buf(&_hash, sizeof(_hash));
}

int HashableHash::cmp(Hashable *h_other)
{
	HashableHash *other = (HashableHash *) h_other;
	return cmp_bufs(&_hash, sizeof(_hash), &other->_hash, sizeof(other->_hash));
}

void HashableHash::increment()
{
	_n_occurrences += 1;
}

uint32_t HashableHash::get_found_at_offset()
{
	return _found_at_offset;
}

uint32_t HashableHash::get_num_occurrences()
{
	return _n_occurrences;
}
