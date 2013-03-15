#include "LiteLib.h"
#include "DataRange.h"
#include "zftp_util.h"

DataRange::DataRange()
{
	this->_start = 0;
	this->_end = 0;
}

DataRange::DataRange(uint32_t start, uint32_t end)
{
	this->_start = start;
	this->_end = end;
}

void DataRange::accumulate(uint32_t start, uint32_t end)
{
	bool worked = maybe_accumulate(start, end);
	lite_assert(worked);
}

void DataRange::accumulate(DataRange other)
{
	accumulate(other.start(), other.end());
}

bool DataRange::maybe_accumulate(uint32_t start, uint32_t end)
{
	if (is_empty())
	{
		this->_start = start;
		this->_end = end;
	}
	else if (end==this->_start)
	{
		this->_start = start;
	}
	else if (start==this->_end)
	{
		this->_end = end;
	}
	else if (start>=_start && end<=_end)
	{
		// blk already in set. Do nothing.
	}
	else
	{
		return false;	// noncontiguous accumulate not representable
	}
	return true;
}

bool DataRange::maybe_accumulate(DataRange other)
{
	return maybe_accumulate(other.start(), other.end());
}

DataRange DataRange::intersect(DataRange other)
{
	DataRange bi;
	if (!(this->is_empty() || other.is_empty()))
	{
		bi._start = max(this->_start, other._start);
		bi._end = min(this->_end, other._end);
		if (bi._start > bi._end)
		{
			bi._start = bi._end = 0;
		}
	}
	return bi;
}

bool DataRange::equals(DataRange *other)
{
	return _start==other->_start && _end==other->_end;
}
