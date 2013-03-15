#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "LiteLib.h"
#include "ShmAllocator.h"

int ShmIdentifier::cmp(LongMessageIdentifierIfc *other)
{
	other = other->base();
	int type_diff = _cheesy_rtti() - other->_cheesy_rtti();
	if (type_diff!=0) { return type_diff; }
	return _key - ((ShmIdentifier *)other)->_key;
}

//////////////////////////////////////////////////////////////////////////////

ShmMessageAllocation::ShmMessageAllocation(LongMessageAllocator *allocator, uint32_t size)
	: LongMessageAllocation(allocator),
	  shm_mapping(allocator->get_shm()->allocate(size)),
	  _id(shm_mapping.get_key())
{ }

ShmMapping ShmMessageAllocation::_ingest(CMShmIdentifier *id, int len)
{
	assert(len == sizeof(*id));
	int shmid = shmget(id->key, id->length, 0600);
	assert(shmid!=-1);

	return ShmMapping(id->length, id->key, shmid);
}

ShmMessageAllocation::ShmMessageAllocation(LongMessageAllocator *allocator, CMShmIdentifier *id, int len)
	: LongMessageAllocation(allocator),
	  shm_mapping(_ingest(id, len)),
	  _id(shm_mapping.get_key())
{ }

ShmMessageAllocation::~ShmMessageAllocation()
{ }

LongMessageIdentifierIfc *ShmMessageAllocation::get_id()
{
	return &_id;
}

void *ShmMessageAllocation::map(void *req_addr)
{
	return shm_mapping.map(req_addr);
}

uint32_t ShmMessageAllocation::get_mapped_size()
{
	return shm_mapping.get_size();
}

void ShmMessageAllocation::unmap()
{
	shm_mapping.unmap();
}

void ShmMessageAllocation::_unlink()
{
	shm_mapping.unlink();
}

int ShmMessageAllocation::marshal(CMLongIdentifier *id, uint32_t capacity)
{
	id->type = CMLongIdentifier::CL_SHM;
	CMShmIdentifier *cshm = &id->un.shm;
	assert(sizeof(*id) <= capacity);
	cshm->key = shm_mapping.get_key();
	cshm->length = shm_mapping.get_size();
	return offsetof(CMLongIdentifier, un) + sizeof(*cshm);
}
