#include "LiteLib.h"
#include "FreshnessSeal.h"

FreshnessSeal::FreshnessSeal()
{
	this->mtime = 0;
}

FreshnessSeal::FreshnessSeal(time_t mtime)
{
	this->mtime = mtime;
}

int FreshnessSeal::cmp(FreshnessSeal *other)
{
	lite_assert(valid());
	lite_assert(other->valid());
	return (int)(mtime-other->mtime);
}

bool FreshnessSeal::valid()
{
	return mtime!=0;
}
