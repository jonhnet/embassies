#include "XWork.h"

XWork::XWork(SyncFactory *sf)
	: event(sf->new_event(false))
{
}
