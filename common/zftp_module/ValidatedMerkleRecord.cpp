#include "ValidatedMerkleRecord.h"

uint32_t ValidatedMerkleRecord::__hash__(const void *a)
{
	ValidatedMerkleRecord *vmr = (ValidatedMerkleRecord *) a;
	return vmr->location();
}

int ValidatedMerkleRecord::__cmp__(const void *a, const void *b)
{
	ValidatedMerkleRecord *vmr_a = (ValidatedMerkleRecord *) a;
	ValidatedMerkleRecord *vmr_b = (ValidatedMerkleRecord *) b;
	return vmr_a->location() - vmr_b->location();
}
