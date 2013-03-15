#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "LiteLib.h"
#include "LongMessageAllocator.h"
#include "MmapAllocator.h"
#include "ShmAllocator.h"

#define DEBUG_LEAKING_LMAS (0)

uint32_t LongMessageIdentifierIfc::hash(const void *datum)
{
	return ((LongMessageIdentifierIfc*)datum)->hash();
}

int LongMessageIdentifierIfc::cmp(const void *v_a, const void *v_b)
{
	LongMessageIdentifierIfc *a = (LongMessageIdentifierIfc *) v_a;
	LongMessageIdentifierIfc *b = (LongMessageIdentifierIfc *) v_b;
	return a->cmp(b);
}

//////////////////////////////////////////////////////////////////////////////

LongMessageAllocation::LongMessageAllocation(LongMessageAllocator *allocator)
	: allocator(allocator)
{
	this->_refs = 1;
	this->mutex = allocator->get_sf()->new_mutex(false);
}

LongMessageAllocation::~LongMessageAllocation()
{
	delete mutex;
}

int LongMessageAllocation::_cheesy_rtti()
{
	assert(false);
	return 0;
}

void LongMessageAllocation::add_ref(const char *dbg_reason)
{
	mutex->lock();
	_refs+=1;
#if DEBUG_LEAKING_LMAS
	if (get_mapped_size() > DEBUG_LEAKING_LMAS)
		{ fprintf(stderr, "lma %p: add_ref(%s) -> %d\n", this, dbg_reason, _refs); }
#endif // DEBUG_LEAKING_LMAS
	mutex->unlock();
}

void LongMessageAllocation::drop_ref(const char *dbg_reason)
{
	mutex->lock();
	lite_assert(_refs>0);
	// TODO mutex
	this->_refs--;
#if DEBUG_LEAKING_LMAS
	if (get_mapped_size() > DEBUG_LEAKING_LMAS)
		{ fprintf(stderr, "lma %p: drop_ref(%s) -> %d\n", this, dbg_reason, _refs); }
#endif // DEBUG_LEAKING_LMAS
	int refs = _refs;
	mutex->unlock();
	if (refs==0)
	{
		unmap();
		allocator->_gone(this);
		delete this;
	}
}

void *LongMessageAllocation::map()
{
	return map(NULL);
}

//////////////////////////////////////////////////////////////////////////////

LongMessageAllocator::LongMessageAllocator(MallocFactory *mf, SyncFactory *sf, bool this_unlinks_shared_buffers, TunIDAllocator* tunid)
	: sf(sf),
	  mutex(sf->new_mutex(true)),
	  this_unlinks_shared_buffers(this_unlinks_shared_buffers),
	  shm(tunid->get_tunid())
{
	hash_table_init(&allocations, mf, LongMessageIdentifierIfc::hash, LongMessageIdentifierIfc::cmp);
}

LongMessageAllocation *LongMessageAllocator::allocate(uint32_t size)
{
	mutex->lock();
	LongMessageAllocation *lma;
	if (0)
	{
		lma = new MmapMessageAllocation(this, size);
	}
	else
	{
		lma = new ShmMessageAllocation(this, size);
	}
	_insert(lma);
	mutex->unlock();
	return lma;
}

LongMessageAllocation *LongMessageAllocator::_ingest_mmap(CMMmapIdentifier *id, int len)
{
	mutex->lock();
	LongMessageAllocation *lma = new MmapMessageAllocation(this, id, len);
	_insert(lma);
	mutex->unlock();
	return lma;
}

LongMessageAllocation *LongMessageAllocator::_ingest_shm(CMShmIdentifier *id, int len)
{
	mutex->lock();
	LongMessageAllocation *lma = new ShmMessageAllocation(this, id, len);
	_insert(lma);
	mutex->unlock();
	return lma;
}

LongMessageAllocation *LongMessageAllocator::ingest(CMLongIdentifier *cmid, int keylen)
{
	if (cmid->type == CMLongIdentifier::CL_MMAP)
	{
		return _ingest_mmap(&cmid->un.mmap, keylen - offsetof(CMLongIdentifier, un));
	}
	else if (cmid->type == CMLongIdentifier::CL_SHM)
	{
		return _ingest_shm(&cmid->un.shm, keylen - offsetof(CMLongIdentifier, un));
	}
	else
	{
		assert(false);
	}
}

void LongMessageAllocator::_insert(LongMessageAllocation *lma)
{
#if DEBUG_LEAKING_LMAS
	fprintf(stderr, "LMA: inserts buf size %d KB\n", lma->get_mapped_size()>>10);
#endif // DEBUG_LEAKING_LMAS
	hash_table_insert(&allocations, lma);
}

class LMAKey
{
private:
	MmapIdentifier *mmap_id;
	ShmIdentifier *shm_id;
	LongMessageIdentifierIfc* the_key;
public:
	LMAKey(CMLongIdentifier* id, int len)
	{
		if (id->type == CMLongIdentifier::CL_MMAP)
		{
			mmap_id = new MmapIdentifier(id->un.mmap.mmap_message_path);
			the_key = mmap_id;
		}
		else if (id->type == CMLongIdentifier::CL_SHM)
		{
			shm_id = new ShmIdentifier(id->un.shm.key);
			the_key = shm_id;
		}
		else
		{
			lite_assert(false);
		}
	}
	~LMAKey() { delete the_key; }
	LongMessageIdentifierIfc* get_key() { return the_key; }
};

LongMessageAllocation* LongMessageAllocator::lookup_existing(CMLongIdentifier* id, int keylen)
{
	LongMessageAllocation* lma;
	mutex->lock();
	LMAKey key(id, keylen);
	lma = (LongMessageAllocation*) hash_table_lookup(&allocations, key.get_key());
	mutex->unlock();
	return lma;
}

void LongMessageAllocator::drop_ref(LongMessageIdentifierIfc *lmid, const char *dbg_reason)
{
	mutex->lock();
	LongMessageAllocation *lma =
		(LongMessageAllocation *) hash_table_lookup(&allocations, lmid);
	lma->drop_ref(dbg_reason);
		// when complete, lma will call my _gone to remove itself from my ht
	mutex->unlock();
}

void LongMessageAllocator::drop_ref(CMLongIdentifier *cmid, int keylen, const char *dbg_reason)
{
	LMAKey key(cmid, keylen);
	drop_ref(key.get_key(), dbg_reason);
}

void LongMessageAllocator::_gone(LongMessageAllocation *lma)
{
	mutex->lock();
	if (this_unlinks_shared_buffers)
	{
#if DEBUG_LEAKING_LMAS
		fprintf(stderr, "LMA: unlinked buf size %d KB\n", lma->get_mapped_size()>>10);
#endif // DEBUG_LEAKING_LMAS
		lma->_unlink();
	}
	hash_table_remove(&allocations, lma);
	mutex->unlock();
}
