#pragma once

#include "CoalescingAllocator.h"

class RangeByStartVoidElt
{
private:
	RangeByStartKey key;

public:
	RangeByStartVoidElt(Range range);

	RangeByStartKey getKey() { return key; }
	Range get_range() { return key.get_range(); }
};

typedef AVLTree<RangeByStartVoidElt,RangeByStartKey> ByStartVoidTree;

class RangeUnion {
private:
	ByStartVoidTree* tree;

public:
	RangeUnion(SyncFactory* sf);
	~RangeUnion();

	void accumulate(Range range);

	// Iteration interface for result
	bool first_range(Range *out_range);
	bool next_range(Range prev_range, Range *out_range);
};
