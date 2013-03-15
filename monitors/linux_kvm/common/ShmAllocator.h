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

#include "pal_abi/pal_types.h"
#include "Shm.h"
#include "CoordinatorProtocol.h"
#include "LongMessageAllocator.h"

class ShmIdentifier : public LongMessageIdentifierIfc {
private:
	int _key;
public:
	ShmIdentifier(int key) : _key(key) {}
	virtual int _cheesy_rtti() { static int x; return (int) &x; }
	virtual uint32_t hash() { return _key; }
	virtual int cmp(LongMessageIdentifierIfc *other);
};

class ShmMessageAllocation : public LongMessageAllocation
{
private:
	ShmMapping shm_mapping;
	ShmIdentifier _id;

	ShmMapping _ingest(CMShmIdentifier *id, int len);

public:
	ShmMessageAllocation(LongMessageAllocator *allocator, uint32_t size);
	ShmMessageAllocation(LongMessageAllocator *allocator, CMShmIdentifier *id, int len);
	~ShmMessageAllocation();

	virtual LongMessageIdentifierIfc *get_id();
	virtual void *map(void *req_addr);
	virtual uint32_t get_mapped_size();
	virtual void unmap();
	virtual void _unlink();

	virtual int marshal(CMLongIdentifier *id, uint32_t capacity);
};
