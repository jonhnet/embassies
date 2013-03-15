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

#include "hash_table.h"
#include "SyncFactory.h"
#include "CoordinatorProtocol.h"
#include "TunIDAllocator.h"
#include "Shm.h"

class LongMessageAllocator;

class LongMessageIdentifierIfc
{
public:
	virtual ~LongMessageIdentifierIfc() {}
	virtual int _cheesy_rtti() = 0;
	virtual uint32_t hash() = 0;
	virtual int cmp(LongMessageIdentifierIfc *other) = 0;
	virtual LongMessageIdentifierIfc *base() { return this; }

	static uint32_t hash(const void *datum);
	static int cmp(const void *a, const void *b);
};

class LongMessageAllocation
	: public LongMessageIdentifierIfc
{
private:
	int _refs;
	SyncFactoryMutex *mutex;

private:
	LongMessageAllocator *allocator;

protected:
	LongMessageAllocation(LongMessageAllocator *allocator);

public:
	~LongMessageAllocation();

	void add_ref(const char *dbg_reason);
	void drop_ref(const char *dbg_reason);

	virtual int _cheesy_rtti();
	virtual uint32_t hash() { return get_id()->hash(); }
	virtual int cmp(LongMessageIdentifierIfc *other)
		{ return get_id()->cmp(other->base()); }
	virtual LongMessageIdentifierIfc *base() { return get_id(); }

	virtual LongMessageIdentifierIfc *get_id() = 0;
	virtual void *map(void *req_addr) = 0;
	virtual uint32_t get_mapped_size() = 0;
	virtual void unmap() = 0;
	virtual int marshal(CMLongIdentifier *id, uint32_t capacity) = 0;

	virtual void _unlink() = 0;

	virtual void *map();
};

class LongMessageAllocator
{
private:
	SyncFactory *sf;
	SyncFactoryMutex *mutex;
	bool this_unlinks_shared_buffers;
	HashTable allocations;
	Shm shm;

	void _insert(LongMessageAllocation *lma);
	LongMessageAllocation *_ingest_mmap(CMMmapIdentifier *id, int len);
	LongMessageAllocation *_ingest_shm(CMShmIdentifier *id, int len);

public:
	LongMessageAllocator(MallocFactory *mf, SyncFactory *sf, bool this_unlinks_shared_buffers, TunIDAllocator* tunid);

	LongMessageAllocation *allocate(uint32_t size);
	LongMessageAllocation *ingest(CMLongIdentifier *id, int keylen);
	LongMessageAllocation *lookup_existing(CMLongIdentifier* id, int keylen);

	void drop_ref(LongMessageIdentifierIfc *lmid, const char *dbg_reason);
	void drop_ref(CMLongIdentifier *id, int keylen, const char *dbg_reason);
	void _gone(LongMessageAllocation *lma);
	SyncFactory *get_sf() { return sf; }
	Shm *get_shm() { return &shm; }
};
