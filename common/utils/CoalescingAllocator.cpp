#include "CoalescingAllocator.h"
#include "math_util.h"

Range::Range()
{
	this->start = 0;
	this->end = 0;
}

Range::Range(uint32_t start, uint32_t end)
{
	this->start = start;
	this->end = end;
}

// jonh TODO: figure out why default copy constructor vanishes when
// I introduce an alternative user-define constructor. Can we get the
// compiler's default copy constructor to hang around so we don't
// have to rewrite (and maintain) it manually?
Range::Range(const Range &r)
{
	this->start = r.start;
	this->end = r.end;
}

Range Range::intersect(const Range &other)
{
	return Range(max(start, other.start), min(end, other.end));
}

int RangeBySizeKey::cmp(Range *a, Range *b)
{
	int v;
	v = a->size() - b->size();
	if (v!=0) { return v; }

	// avl2 demands unique keys, so resolve ties with start
	v = a->start - b->start;
	return v;
}

RangeBySizeElt::RangeBySizeElt(Range range)
	: key(range)
{}

int RangeByStartKey::cmp(Range *a, Range *b)
{
	int v;
	v = a->start - b->start;
	return v;
}

RefcountedUserObj::RefcountedUserObj(UserObjIfc *obj)
{
	this->obj = obj;
	this->refcount = 0;
}

RefcountedUserObj::~RefcountedUserObj()
{
	delete this->obj;	// invoke user's virtual dtor.
}

void RefcountedUserObj::add_ref()
{
	this->refcount += 1;
}

void RefcountedUserObj::drop_ref()
{
	this->refcount -= 1;
	if (this->refcount == 0)
	{
		delete this;
	}
}

RangeByStartElt::RangeByStartElt(Range range, RefcountedUserObj *refcounted_user_obj)
	: key(range), refcounted_user_obj(refcounted_user_obj)
{
	if (refcounted_user_obj!=NULL)
	{
		refcounted_user_obj->add_ref();
	}
}

RangeByStartElt::~RangeByStartElt()
{
	if (refcounted_user_obj!=NULL)
	{
		refcounted_user_obj->drop_ref();
	}
}

UserObjIfc *RangeByStartElt::get_user_obj()
{
	if (refcounted_user_obj==NULL)
	{
		return NULL;
	}
	return refcounted_user_obj->get_user_obj();
}

CoalescingAllocator::CoalescingAllocator(SyncFactory *sf, bool allow_redundant_frees)
{
	this->mutex = sf->new_mutex(false);
	this->allow_redundant_frees = allow_redundant_frees;

	// TODO don't really need mutexes over subtrees, since we use a object-size mutex. Could create a no-op mutex SyncFactoryMutex class?
	free_by_size_tree = new BySizeTree(sf);
	free_by_start_tree = new ByStartTree(sf);
	allocated_tree = new ByStartTree(sf);
}

CoalescingAllocator::~CoalescingAllocator()
{
	lite_assert(allocated_tree->size()==0);	// I guess we could walk & free all the user_objs...
	delete mutex;
}

void CoalescingAllocator::create_empty_range(Range range)
{
	mutex->lock();
	_create_empty_range_nolock(range);
	mutex->unlock();
}

void CoalescingAllocator::_remove_free_range_by_start_elt(RangeByStartElt *start_elt)
{
	RangeBySizeKey size_key(start_elt->getKey().get_range());
	RangeBySizeElt *corresponding_elt_by_size =
		free_by_size_tree->lookup(size_key);
	lite_assert(corresponding_elt_by_size!=NULL);	// failed invariant that each free range appears in both trees.
	free_by_size_tree->remove(corresponding_elt_by_size);

	free_by_start_tree->remove(start_elt);
}

void CoalescingAllocator::_insert_coalesced_free_range(Range range)
{
	bool rc;
	lite_assert(range.size()>0);
	rc = free_by_size_tree->insert(new RangeBySizeElt(range));
	lite_assert(rc);
	rc = free_by_start_tree->insert(new RangeByStartElt(range, NULL));
	lite_assert(rc);
}

void CoalescingAllocator::_create_empty_range_nolock(Range new_range)
{
	// a failure would be
	// an existing range that starts after this one starts,
	//	but before this new one ends
	RangeByStartKey new_range_key(new_range);
	RangeByStartElt *following_elt =
		free_by_start_tree->findFirstGreaterThanOrEqualTo(new_range_key);
	if (following_elt!=NULL)
	{
		Range following_range = following_elt->getKey().get_range();
		lite_assert(following_range.start >= new_range.end);	// uh-oh -- there's already a free range here!
		if (following_range.start == new_range.end)
		{
			// coalesce new range with this range.
			_remove_free_range_by_start_elt(following_elt);
			delete following_elt;
			Range union_range = Range(new_range.start, following_range.end);
			// NB This recursion will terminate, because it
			// *won't* take this first branch, because there won't
			// be a right-side coalescing opportunity, because
			// (by invariant) the previous insert of free space
			// already coalesced it.
			_create_empty_range_nolock(union_range);
			// And we don't need to continue in this call frame
			// to coalesce to the left, because the recursive
			// call above just took care of that.
			return;
		}
	}

	RangeByStartElt *preceding_elt =
		free_by_start_tree->findFirstLessThanOrEqualTo(new_range_key);
	if (preceding_elt!=NULL)
	{
		Range preceding_range = preceding_elt->getKey().get_range();
		lite_assert(preceding_range.end <= new_range.start);	// uh-oh -- there's already a free range here!
		if (preceding_range.end == new_range.start)
		{
			// coalesce new range with this range.
			_remove_free_range_by_start_elt(preceding_elt);
			delete preceding_elt;
			Range union_range = Range(preceding_range.start, new_range.end);

			// By the time we get here, we either didn't need to
			// right-coalesce, or we did in the calling stack frame,
			// so that's taken care of.
			_insert_coalesced_free_range(union_range);
			// and we don't want to fall through to insert again.
			return;
		}
	}

	_insert_coalesced_free_range(new_range);
}

bool CoalescingAllocator::allocate_range(uint32_t size, UserObjIfc *user_obj, Range *out_range)
{
	if (size==0)
	{
		return false;
	}

	mutex->lock();

	bool result = false;
	// make a key whose size hits the target, and that
	// tie-breaks <= all ranges of equal size (start == 0)
	RangeBySizeKey size_key(Range(0, size));

	RangeBySizeElt *smallest_suitable_range =
		free_by_size_tree->findFirstGreaterThanOrEqualTo(size_key);
	if (smallest_suitable_range == NULL)
	{
		// sorry, no range available.
		result = false;
	}
	else
	{
		Range assigned_range = smallest_suitable_range->getKey().get_range();
		free_by_size_tree->remove(smallest_suitable_range);
		
		Range consumed_range(assigned_range.start, assigned_range.start+size);
		lite_assert(consumed_range.intersect(assigned_range).size()==size);
		Range leftover_range(consumed_range.end, assigned_range.end);

		RangeByStartKey start_key(consumed_range);
		RangeByStartElt *corresponding_range_by_start =
			free_by_start_tree->lookup(start_key);
		lite_assert(corresponding_range_by_start != NULL); // invariant: each free range appears in both free trees.
		free_by_start_tree->remove(corresponding_range_by_start);

		RefcountedUserObj *refcounted_user_obj = new RefcountedUserObj(user_obj);
		RangeByStartElt *user_range = new RangeByStartElt(
			consumed_range, refcounted_user_obj);

		allocated_tree->insert(user_range);

		delete smallest_suitable_range;
		delete corresponding_range_by_start;

		if (leftover_range.size()>0)
		{
			_create_empty_range_nolock(leftover_range);
		}

		*out_range = consumed_range;
		result = true;
	}

	mutex->unlock();
	return result;
}

UserObjIfc *CoalescingAllocator::lookup_value(uint32_t value, Range *out_range)
{
	Range req_range(value, value+1);
	RangeByStartKey start_key(req_range);
	RangeByStartElt *elt =
		allocated_tree->findFirstLessThanOrEqualTo(start_key);
	if (elt==NULL)
	{
		return NULL;
	}
	Range found_range = elt->getKey().get_range();
	if (value>=found_range.start && value<found_range.end)
	{
		if (out_range!=NULL)
		{
			*out_range = found_range;
		}
		return elt->get_user_obj();
	}
	return NULL;
}

void CoalescingAllocator::_free_subrange(RangeByStartElt *allocated, Range overlap)
{
	allocated_tree->remove(allocated);

	Range allocated_range = allocated->getKey().get_range();
	Range left(allocated_range.start, overlap.start);
	Range right(overlap.end, allocated_range.end);

	// NB each ctor takes a reference, to account for now-split allocation.
	if (left.size()>0)
	{
		RangeByStartElt *left_fragment =
			new RangeByStartElt(left, allocated->refcounted_user_obj);
		allocated_tree->insert(left_fragment);
	}

	if (right.size()>0)
	{
		RangeByStartElt *right_fragment =
			new RangeByStartElt(right, allocated->refcounted_user_obj);
		allocated_tree->insert(right_fragment);
	}
	// That done, we now give up the original reference.
	delete allocated;

	// now put the middle onto the free trees
	_create_empty_range_nolock(overlap);
}

void CoalescingAllocator::free_range(Range in_range)
{
	mutex->lock();

	Range range = in_range;
	while (range.size() > 0)
	{
		RangeByStartKey start_key(range);
		RangeByStartElt *first_elt =
			allocated_tree->findFirstLessThanOrEqualTo(start_key);
		Range overlap = first_elt==NULL
			? Range(0,0)
			: range.intersect(first_elt->getKey().get_range());
		if (overlap.size()==0)
		{
			// there's a gap between requested range.start and first
			// range that overlaps (if any).
			// Advance range to next key forward, skipping over non-overlapping
			// region.
			lite_assert(allow_redundant_frees);
			RangeByStartElt *next_elt =
				allocated_tree->findFirstGreaterThanOrEqualTo(start_key);
			if (next_elt==NULL)
			{
				// Oh, we're done -- there are no allocated ranges left
				// to reclaim.
				break;
			}
			lite_assert(next_elt->getKey() > start_key); // or else I misunderstood the 'findFirstLessThanOrEqualTo' above!
			range.start = next_elt->getKey().get_range().start;
			continue;
		}

		_free_subrange(first_elt, overlap);
		range.start = overlap.end;
	}

	mutex->unlock();
}

UserObjIfc *CoalescingAllocator::first_range(Range *out_range)
{
	RangeByStartElt *elt = allocated_tree->findMin();
	if (elt!=NULL)
	{
		*out_range = elt->getKey().get_range();
		return elt->get_user_obj();
	}
	else
	{
		return NULL;
	}
}

UserObjIfc *CoalescingAllocator::next_range(Range prev_range, Range *out_range)
{
	RangeByStartKey key(prev_range);
	RangeByStartElt *elt = allocated_tree->findFirstGreaterThan(key);
	if (elt!=NULL)
	{
		*out_range = elt->getKey().get_range();
		return elt->get_user_obj();
	}
	else
	{
		return NULL;
	}
}

