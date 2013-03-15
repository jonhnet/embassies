#include "LiteLib.h"
#include "SyncFactory_Zutex.h"
#include "xax_util.h"

SyncFactoryMutex_Zutex::SyncFactoryMutex_Zutex(ZoogDispatchTable_v1 *zdt)
{
	this->zdt = zdt;
	zmutex_init(&zmutex);
}

SyncFactoryMutex_Zutex::~SyncFactoryMutex_Zutex()
{
	zmutex_free(&zmutex);
}

void SyncFactoryMutex_Zutex::lock()
{
	zmutex_lock(zdt, &zmutex);
}

void SyncFactoryMutex_Zutex::unlock()
{
	zmutex_unlock(zdt, &zmutex);
}

//////////////////////////////////////////////////////////////////////////////

SyncFactoryRecursiveMutex_Zutex::SyncFactoryRecursiveMutex_Zutex(ZoogDispatchTable_v1 *zdt)
{
	this->zdt = zdt;
	this->owner_id = 0;
	this->depth = 0;
	zmutex_init(&owner_id_zmutex);
	zmutex_init(&main_zmutex);
}

SyncFactoryRecursiveMutex_Zutex::~SyncFactoryRecursiveMutex_Zutex()
{
	zmutex_free(&owner_id_zmutex);
	zmutex_free(&main_zmutex);
}

void SyncFactoryRecursiveMutex_Zutex::lock()
{
	uint32_t my_thread_id = get_gs_base();
	zmutex_lock(zdt, &owner_id_zmutex);
	if (owner_id==my_thread_id)
	{
		// I'm already holding the inner lock, above. Proceed.
		depth++;
	}
	else
	{
		// TODO this implementation is scary. :v(
		zmutex_lock(zdt, &main_zmutex);
		owner_id = my_thread_id;
		depth = 1;
	}
	zmutex_unlock(zdt, &owner_id_zmutex);
}

void SyncFactoryRecursiveMutex_Zutex::unlock()
{
	zmutex_lock(zdt, &owner_id_zmutex);
	depth--;
	if (depth==0)
	{
		zmutex_unlock(zdt, &main_zmutex);
		owner_id = 0;
	}
	zmutex_unlock(zdt, &owner_id_zmutex);
}

//////////////////////////////////////////////////////////////////////////////

SyncFactoryEvent_Zutex::SyncFactoryEvent_Zutex(
	ZoogDispatchTable_v1 *zdt,
	bool auto_reset)
{
	this->zdt = zdt;
	zevent_init(&zevent, auto_reset);
}

SyncFactoryEvent_Zutex::~SyncFactoryEvent_Zutex()
{
	zevent_free(&zevent);
}

void SyncFactoryEvent_Zutex::wait()
{
	ZAS *zasp = &zevent.zas;
	zas_wait_any(zdt, &zasp, 1);
}

bool SyncFactoryEvent_Zutex::wait(int timeout_ms)
{
	lite_assert(false);	// not available here; see SyncFactory_TimedZutex.
	return false;
}

void SyncFactoryEvent_Zutex::signal()
{
	zevent_signal(zdt, &zevent, NULL);
}

void SyncFactoryEvent_Zutex::reset()
{
	zevent_reset(zdt, &zevent);
}

SyncFactory_Zutex::SyncFactory_Zutex(ZoogDispatchTable_v1 *zdt)
{
	this->zdt = zdt;
}

SyncFactoryMutex *SyncFactory_Zutex::new_mutex(bool recursive)
{
	if (recursive)
	{
		return new SyncFactoryRecursiveMutex_Zutex(zdt);
	}
	else
	{
		return new SyncFactoryMutex_Zutex(zdt);
	}
}

SyncFactoryEvent *SyncFactory_Zutex::new_event(bool auto_reset)
{
	return new SyncFactoryEvent_Zutex(zdt, auto_reset);
}

