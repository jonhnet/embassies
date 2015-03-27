#include <malloc.h>
#include <string.h>
#include "ChunkRecord.h"

ChunkRecord::ChunkRecord(const char* url, bool own_url, DataRange data_range, uint32_t location)
	: own_url(own_url),
	  data_range(data_range),
	  location(location)
{
	if (own_url)
	{
		this->url = strdup(url);
	}
	else
	{
		this->url = (char*) url;
	}
}

ChunkRecord::ChunkRecord(const ChunkRecord& other)
	: url(other.url),
	  own_url(false),
	  data_range(other.data_range),
	  location(other.location)
{
}

ChunkRecord& ChunkRecord::operator=(ChunkRecord other)
{
	url = other.url;
	own_url = false;
	data_range = other.data_range;
	location = other.location;
	return *this;
}

ChunkRecord::~ChunkRecord()
{
	if (own_url)
	{
		free(url);
	}
}

int ChunkRecord::cmp(ChunkRecord *other)
{
	int r;
	r = - (data_range.size() - other->data_range.size());
	if (r!=0) { return r; }
	r = strcmp(url, other->url);
	if (r!=0) { return r; }
	r = location - other->location;
	return r;
}

bool ChunkRecord::content_equals(ChunkRecord *other)
{
	int r;
	r = - (data_range.size() - other->data_range.size());
	if (r!=0) { return false; }
	r = strcmp(url, other->url);
	if (r!=0) { return false; }
	return true;
}

