/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
#pragma once

#include "SyncFactory.h"
#include "avl2.h"

class Range
{
public:
	Range();
	Range(uint32_t start, uint32_t end);
	Range(const Range &r);

	Range intersect(const Range &other);
	uint32_t start;
	uint32_t end;
	// this object represents the range [start,end), end not included.
	// So start=0x5000,end=0x6000 is a 0x1000-byte range whose last
	// byte is at 0x5ffff.
	inline uint32_t size() { return (end < start) ? 0 : end-start; }
};

class RangeBySizeKey
{
public:
	RangeBySizeKey(const Range &range) : range(range) {}
	RangeBySizeKey(const RangeBySizeKey &rbse) : range(rbse.range) {}
	int cmp(Range *a, Range *b);

	bool operator<(RangeBySizeKey b)
		{ return cmp(&range, &b.range)<0; }
	bool operator>(RangeBySizeKey b)
		{ return cmp(&range, &b.range)>0; }
	bool operator==(RangeBySizeKey b)
		{ return cmp(&range, &b.range)==0; }
	bool operator<=(RangeBySizeKey b)
		{ return cmp(&range, &b.range)<=0; }
	bool operator>=(RangeBySizeKey b)
		{ return cmp(&range, &b.range)>=0; }

	Range get_range() { return range; }
private:
	Range range;
};

class RangeBySizeElt
{
public:
	// used only for free ranges, so no user 'datum' field
	RangeBySizeElt(Range range);

	RangeBySizeKey key;
	RangeBySizeKey getKey() { return key; }
};

typedef AVLTree<RangeBySizeElt,RangeBySizeKey> BySizeTree;

class RangeByStartKey
{
public:
	RangeByStartKey(const Range &range) : range(range) {}
	RangeByStartKey(const RangeByStartKey &rbse) : range(rbse.range) {}
	int cmp(Range *a, Range *b);

	bool operator<(RangeByStartKey b)
		{ return cmp(&range, &b.range)<0; }
	bool operator>(RangeByStartKey b)
		{ return cmp(&range, &b.range)>0; }
	bool operator==(RangeByStartKey b)
		{ return cmp(&range, &b.range)==0; }
	bool operator<=(RangeByStartKey b)
		{ return cmp(&range, &b.range)<=0; }
	bool operator>=(RangeByStartKey b)
		{ return cmp(&range, &b.range)>=0; }

	Range get_range() { return range; }
private:
	Range range;
};

// We'll call this when we free last instance of a user obj
class UserObjIfc
{
public:
	virtual ~UserObjIfc() {}
};

class RefcountedUserObj
{
public:
	RefcountedUserObj(UserObjIfc *obj);
		// starts at refcount==0

	void add_ref();
	void drop_ref();
		// deletes this and obj when last ref removed.
	UserObjIfc *get_user_obj() { return obj; }

private:
	~RefcountedUserObj();

	UserObjIfc *obj;
	int refcount;
};

class RangeByStartElt
{
public:
	RangeByStartElt(Range range, RefcountedUserObj *refcounted_user_obj);
	~RangeByStartElt();

	RangeByStartKey getKey() { return key; }
	UserObjIfc *get_user_obj();

private:
	RangeByStartKey key;
public:
	RefcountedUserObj *refcounted_user_obj;
};

typedef AVLTree<RangeByStartElt,RangeByStartKey> ByStartTree;


class CoalescingAllocator
{
public:
	CoalescingAllocator(SyncFactory *sf, bool allow_redundant_frees);
	~CoalescingAllocator();

	void create_empty_range(Range range);
		// asserts that the new range doesn't overlap any existing
		// free or allocated range.
	bool allocate_range(uint32_t size, UserObjIfc *user_obj, Range *out_range);
		// returns false if no suitable range available.
		// if it returns false, then user_obj is still owned by caller;
		// otherwise, we accept responsibility to delete it once all
		// the subranges of this allocation are freed.
	UserObjIfc *lookup_value(uint32_t value, Range *out_range);
		// If the range containing 'value' is allocated, return the associated
		// UserObjIfc, and indicate the range in out_range.
		// Otherwise returns null, and out_range is not updated.
	void free_range(Range range);
		// asserts range was all allocated

	// Iteration interface for allocations
	UserObjIfc *first_range(Range *out_range);
	UserObjIfc *next_range(Range prev_range, Range *out_range);

	ByStartTree *dbg_peek_allocated_tree() { return allocated_tree; }
	ByStartTree *dbg_peek_free_by_start_tree() { return free_by_start_tree; }

private:
	void _check_no_overlap(Range new_range, ByStartTree *tree);
	void _insert_coalesced_free_range(Range range);
	void _remove_free_range_by_start_elt(RangeByStartElt *start_elt);
	void _create_empty_range_nolock(Range range);
	void _free_subrange(RangeByStartElt *allocated, Range overlap);

	bool allow_redundant_frees;
	SyncFactoryMutex *mutex;

	BySizeTree *free_by_size_tree;
	ByStartTree *free_by_start_tree;
	ByStartTree *allocated_tree;
};
