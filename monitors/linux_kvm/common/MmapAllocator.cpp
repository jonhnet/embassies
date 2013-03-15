#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "LiteLib.h"
#include "MmapAllocator.h"

MmapIdentifier::MmapIdentifier(char *name)
	: name(strdup(name))
{
}

MmapIdentifier::~MmapIdentifier()
{
	free(name);
}

uint32_t MmapIdentifier::hash()
{
	return hash_buf(name, strlen(name));
}

int MmapIdentifier::cmp(LongMessageIdentifierIfc *other)
{
	other = other->base();
	int type_diff = _cheesy_rtti() - other->_cheesy_rtti();
	if (type_diff!=0) { return type_diff; }
	MmapIdentifier *m_other = (MmapIdentifier *) other;
	return strcmp(name, m_other->name);
}

//////////////////////////////////////////////////////////////////////////////

MmapMessageAllocation::MmapMessageAllocation(LongMessageAllocator *allocator, uint32_t size)
	: LongMessageAllocation(allocator),
	  mapped(NULL)
{
	char buf[1000];
	sprintf(buf, "/tmp/zoog_msg_from_coordinator.XXXXXX");
	open_fd = mkstemp(buf);
	lite_assert(open_fd>=0);

	int rc = ftruncate(open_fd, size);
	assert(rc==0);

	_size = size;

	_id = new MmapIdentifier(buf);
}

MmapMessageAllocation::MmapMessageAllocation(LongMessageAllocator *allocator, CMMmapIdentifier *id, int len)
	: LongMessageAllocation(allocator),
	  mapped(NULL)
{
	open_fd = open(id->mmap_message_path, O_RDONLY);
	assert(open_fd>=0);

	struct stat statbuf;
	int rc = fstat(open_fd, &statbuf);
	lite_assert(rc==0);
	_size = statbuf.st_size;

	assert((uint32_t) len == offsetof(CMLongIdentifier, un)+strlen(id->mmap_message_path)+1);
	_id = new MmapIdentifier(id->mmap_message_path);
}

MmapMessageAllocation::~MmapMessageAllocation()
{
	close(open_fd);
	delete _id;
}


LongMessageIdentifierIfc *MmapMessageAllocation::get_id()
{
	return _id;
}

void *MmapMessageAllocation::map(void *req_addr)
{
	if (mapped==NULL)
	{
		assert(false);	// mmap deprecated; should be using shm now
		mapped = mmap(req_addr, _size, PROT_READ, MAP_SHARED, open_fd, 0);
		assert(mapped != MAP_FAILED);
		assert(req_addr==NULL || mapped==req_addr);
	}
	return mapped;
}

void MmapMessageAllocation::unmap()
{
	int rc = munmap(mapped, _size);
	lite_assert(rc==0);
}

void MmapMessageAllocation::_unlink()
{
	int rc = unlink(_id->get_name());
	lite_assert(rc==0);
}

int MmapMessageAllocation::marshal(CMLongIdentifier *id, uint32_t capacity)
{
	id->type = CMLongIdentifier::CL_MMAP;
	CMMmapIdentifier *mshm = &id->un.mmap;
	uint32_t len = offsetof(CMLongIdentifier, un) + sizeof(*mshm)+strlen(_id->get_name())+1;
	assert(len <= capacity);
	memcpy(mshm->mmap_message_path, _id->get_name(), strlen(_id->get_name())+1);
	return len;
}
