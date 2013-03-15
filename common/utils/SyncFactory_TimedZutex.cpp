#include "LiteLib.h"
#include "SyncFactory_TimedZutex.h"
#include "xax_util.h"


SyncFactoryEvent_TimedZutex::SyncFactoryEvent_TimedZutex(
	ZoogDispatchTable_v1 *zdt,
	bool auto_reset,
	ZClock *zclock)
	: SyncFactoryEvent_Zutex(zdt, auto_reset),
	  zclock(zclock),
	  ztimer(NULL)
{
}

bool SyncFactoryEvent_TimedZutex::wait(int timeout_ms)
{
	lite_assert(zclock!=NULL);

	uint64_t absolute_time = zclock->get_time_zoog();
	absolute_time += (((uint64_t) timeout_ms)* 1000000LL);
	ZTimer ztimer(zclock, absolute_time);
	
	ZAS *zasses[2];
	zasses[0] = &zevent.zas;
	zasses[1] = ztimer.get_zas();
	zas_wait_any(zdt, zasses, 2);
	return zevent_is_set(zdt, &zevent);
}

SyncFactory_TimedZutex::SyncFactory_TimedZutex(ZoogDispatchTable_v1 *zdt, ZClock *zclock)
	: SyncFactory_Zutex(zdt),
	  zclock(zclock)
{
}

SyncFactoryEvent *SyncFactory_TimedZutex::new_event(bool auto_reset)
{
	return new SyncFactoryEvent_TimedZutex(zdt, auto_reset, zclock);
}

