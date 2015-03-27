#include "RangeUnion.h"
#include "math_util.h"

RangeByStartVoidElt::RangeByStartVoidElt(Range range)
	: key(range) { }

RangeUnion::RangeUnion(SyncFactory* sf)
{
	tree = new ByStartVoidTree(sf);
}

RangeUnion::~RangeUnion()
{
	lite_assert(tree->size()==0);	// I guess we could walk & free all the user_objs...
}

void RangeUnion::accumulate(Range range)
{
	while (true)
	{
		RangeByStartVoidElt* left =
			tree->findFirstLessThanOrEqualTo(range);
		if (left!=NULL && left->get_range().end >= range.start)
		{
			tree->remove(left);
			range = Range(
				left->get_range().start, max(left->get_range().end, range.end));
			delete left;
			continue;
		}

		RangeByStartVoidElt* right =
			tree->findFirstGreaterThanOrEqualTo(range);
		if (right!=NULL && right->get_range().start <= range.end)
		{
			tree->remove(right);
			range = Range(
				range.start, max(right->get_range().end, range.end));
			delete right;
			continue;
		}

		tree->insert(new RangeByStartVoidElt(range));
		break;
	}
}

// Iteration interface for result
bool RangeUnion::first_range(Range *out_range)
{
	RangeByStartVoidElt* first = tree->findMin();
	if (first==NULL) { return false; }
	*out_range = first->get_range();
	return true;
}

bool RangeUnion::next_range(Range prev_range, Range *out_range)
{
	RangeByStartVoidElt* next = tree->findFirstGreaterThan(prev_range);
	if (next==NULL) { return false; }
	*out_range = next->get_range();
	return true;
}
