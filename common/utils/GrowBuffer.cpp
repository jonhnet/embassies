#include "LiteLib.h"
#include "GrowBuffer.h"

GrowBuffer::GrowBuffer(MallocFactory *mf)
{
	this->mf = mf;
	capacity = 512;
	buf = (char*) mf_malloc(mf, capacity);
	size = 0;
}

GrowBuffer::~GrowBuffer()
{
	mf_free(mf, buf);
}

void GrowBuffer::_grow(uint32_t min_capacity)
{
	uint32_t goal_capacity = capacity;
	while (min_capacity > goal_capacity)
	{
		goal_capacity *= 2;
	}
	char *buf2 = (char*) mf_malloc(mf, goal_capacity);
	lite_memcpy(buf2, buf, capacity);
	mf_free(mf, buf);
	buf = buf2;
	capacity = goal_capacity;
}

void GrowBuffer::append(const char *msg)
{
	int len = lite_strlen(msg);
	_grow(size+len+1);	// leave a terminator in place, but don't count it in size.
	lite_memcpy(buf+size, msg, len+1);
	size += len;
}
