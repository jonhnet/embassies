#include <sys/syscall.h>
#include <linux/futex.h>

#include "LiteLib.h"
#include "gsfree_syscall.h"
#include "SyncFactory_GsfreeFutex.h"

SyncFactory_GsfreeFutex::SyncFactory_GsfreeFutex()
{
	lite_memset(&fake_zutex_dt, 0, sizeof(fake_zutex_dt));
	fake_zutex_dt.zoog_zutex_wait = zoog_zutex_wait;
	fake_zutex_dt.zoog_zutex_wake = zoog_zutex_wake;
	sf_zutex = new SyncFactory_Zutex(&fake_zutex_dt);
}

SyncFactoryMutex *SyncFactory_GsfreeFutex::new_mutex(bool recursive)
{
	return sf_zutex->new_mutex(recursive);
}

SyncFactoryEvent *SyncFactory_GsfreeFutex::new_event(bool auto_reset)
{
	return sf_zutex->new_event(auto_reset);
}

void SyncFactory_GsfreeFutex::zoog_zutex_wait(ZutexWaitSpec *specs, uint32_t count)
{
	lite_assert(count==1);
	_gsfree_syscall(__NR_futex, specs[0].zutex, FUTEX_WAIT, specs[0].match_val, NULL);
}

int SyncFactory_GsfreeFutex::zoog_zutex_wake(
    uint32_t *zutex,
    uint32_t match_val, 
    Zutex_count n_wake,
    Zutex_count n_requeue,
    uint32_t *requeue_zutex,
    uint32_t requeue_match_val)
{
	lite_assert(n_requeue == 0);
	return _gsfree_syscall(__NR_futex, zutex, FUTEX_WAKE, n_wake);
}
