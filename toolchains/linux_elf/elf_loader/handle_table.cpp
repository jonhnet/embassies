#include "LiteLib.h"
#include "handle_table.h"
#include "xax_util.h"
#include "xax_posix_emulation.h"	// init_xfs_handle; could factor out?
#include "XVFSHandleWrapper.h"

// TODO this is a goofy implementation dating back to the days when
// we didn't have any data structures. Probably want to use a
// CoalescingAllocator or a HashTable(in use)/LinkedList (free).

void HandleTable::_add_to_free_list_lockheld(int filenum)
{
	xax_assert(filenum >=0 && filenum < capacity);
	entries[filenum].next_free = first_free;
	entries[filenum].state = ht_free;
	first_free = filenum;
}

void HandleTable::_expand_lockheld()
{
	static int reenter = 0;

	xax_assert(!reenter);
	reenter = 1;

	int old_capacity = capacity;
	int new_capacity = capacity * 2;
	if (new_capacity < 10)
	{
		new_capacity = 10;
	}

#define HT_EXPAND_MALLOC(size)	(mf->c_malloc)(mf, size)
#define HT_EXPAND_FREE(ptr)		(mf->c_free)(mf, ptr)
//#define HT_EXPAND_MALLOC(size)	malloc(size)
//#define HT_EXPAND_FREE(ptr)		free(ptr)
	uint32_t size = sizeof(HandleTableEntry)*new_capacity;
	HandleTableEntry *new_entries = (HandleTableEntry*) HT_EXPAND_MALLOC(size);

	if (entries!=NULL)
	{
		lite_memcpy(new_entries, entries, sizeof(HandleTableEntry)*old_capacity);
		HT_EXPAND_FREE(entries);
	}

	capacity = new_capacity;
	entries = new_entries;
	int i;
	// add backwards to cause lowest numbers to be assigned first.
	// needed because of init assignment to fds 0, 1, 2
	for (i=new_capacity - 1; i >= old_capacity; i--)
	{
		_add_to_free_list_lockheld(i);
		counters[ht_free] += 1;
	}

	reenter = 0;
}

HandleTable::HandleTable(MallocFactory *mf)
{
	this->mf = mf;

	this->capacity = 0;
	this->counters[ht_free] = 0;
	this->counters[ht_assigned] = 0;
	this->counters[ht_used] = 0;
	this->entries = NULL;
	this->first_free = -1;

	cheesy_lock_init(&this->entries_lock);
}

void HandleTable::_transition_lockheld(int filenum, HTState s0, HTState s1)
{
	HTState old_state = entries[filenum].state;
	if (s0==ht_notfree)
	{
		xax_assert(old_state == ht_used
			|| old_state == ht_assigned);
	}
	else
	{
		xax_assert(old_state == s0);
	}
	entries[filenum].state = s1;
	counters[old_state] -= 1;
	counters[s1] += 1;
}

int HandleTable::allocate_filenum()
{
	cheesy_lock_acquire(&entries_lock);
	int filenum;
	if (first_free == -1)
	{
		_expand_lockheld();
	}
	xax_assert(first_free != -1);
	filenum = first_free;
	first_free = entries[filenum].next_free;
	_transition_lockheld(filenum, ht_free, ht_assigned);
	cheesy_lock_release(&entries_lock);

	return filenum;
}

void HandleTable::assign_filenum(int filenum, XVFSHandleWrapper *wrapper)
{
	cheesy_lock_acquire(&entries_lock);
	_transition_lockheld(filenum, ht_assigned, ht_used);
	entries[filenum].wrapper = wrapper;
	cheesy_lock_release(&entries_lock);
}

void HandleTable::set_nonblock(int filenum) {
	XVFSHandleWrapper *wrapper = lookup(filenum);
	wrapper->status_flags |= O_NONBLOCK;
}

void HandleTable::assign_filenum(int filenum, XaxVFSHandleIfc *hdl)
{
	assign_filenum(filenum, new XVFSHandleWrapper(hdl));
}

void HandleTable::free_filenum(int filenum)
{
	cheesy_lock_acquire(&entries_lock);
	_transition_lockheld(filenum, ht_notfree, ht_free);
	_add_to_free_list_lockheld(filenum);
	cheesy_lock_release(&entries_lock);
}

XVFSHandleWrapper *HandleTable::lookup(int filenum)
{
	XVFSHandleWrapper *result;

	cheesy_lock_acquire(&entries_lock);
	if (filenum<0 || filenum >= capacity)
	{
		result = NULL;
	}
	else if (entries[filenum].state != ht_used)
	{
		result = NULL;
	}
	else
	{
		result = entries[filenum].wrapper;
	}
	cheesy_lock_release(&entries_lock);
	return result;
}


