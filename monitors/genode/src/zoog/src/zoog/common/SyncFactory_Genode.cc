#include <base/env.h>
#include <base/printf.h>
#include <base/sleep.h>

using namespace Genode;

#include "common/SyncFactory_Genode.h"
#include "LiteLib.h"

SyncFactoryMutex_Genode::SyncFactoryMutex_Genode(bool recursive)
	: depth(0),
	  holder(Thread_id::get_invalid())
{
	// genode locks are always recursive. Thanks, Genode!
}

void SyncFactoryMutex_Genode::lock()
{
	Thread_id myself = Thread_id::get_my();
	fast.lock();
	Thread_id holder_local = this->holder;
	fast.unlock();

	if (holder_local == myself)
	{
		// great -- no one else can change fast, or read/write depth,
		// without holding the blocking lock; we already hold that.
		depth += 1;
		return;
	}

	// take the blocking lock...
	blocking.lock();
	// now we can't take the fast lock before the blocking lock,
	// because unlock requires the opposite lock order discipline;
	// so to avoid deadlock, we keep this order.
	fast.lock();
	// ...but now we know that no one else has the lock, else they'd
	// be holding blocking.
	Thread_id inv = Thread_id::get_invalid();
	lite_assert(holder == inv)
	lite_assert(depth == 0)
	holder = myself;
	depth = 1;
	fast.unlock();
	// ...we keep blocking until we complete the recursive unlock.
}

void SyncFactoryMutex_Genode::unlock()
{
	Thread_id myself = Thread_id::get_my();
	fast.lock();
	lite_assert(holder == myself);
	depth -= 1;
	if (depth==0)
	{
		holder = Thread_id::get_invalid();
		blocking.unlock();
	}
	fast.unlock();
}

SyncFactoryEvent_Genode::SyncFactoryEvent_Genode(bool auto_reset)
	: _signaled(false),
	  _auto_reset(auto_reset)
{
}

void SyncFactoryEvent_Genode::wait()
{
	bool signaled_local;
	_lock.lock();
	signaled_local = _signaled;
	_lock.unlock();
	if (!signaled_local) {
		_sem.down();
	}
}

bool SyncFactoryEvent_Genode::wait(int timeout_ms)
{
	lite_assert(false);
	return false;
}

void SyncFactoryEvent_Genode::signal()
{
	_lock.lock();
	if (!_auto_reset) {
		_signaled = true;
	}
	else {
		//PDBG("_auto_reset %d _signaled %d\n", _auto_reset, _signaled);
		lite_assert(!_signaled);
	}
	while (_sem.cnt() < 0) {
		// wake everybody up on a signal.
		_sem.up();
	}
	_lock.unlock();
}

void SyncFactoryEvent_Genode::reset()
{
	lite_assert(!_auto_reset);
	_lock.lock();
	_signaled = false;
	_lock.unlock();
}

SyncFactoryMutex *SyncFactory_Genode::new_mutex(bool recursive)
{
	return new SyncFactoryMutex_Genode(recursive);
}

SyncFactoryEvent *SyncFactory_Genode::new_event(bool auto_reset)
{
	return new SyncFactoryEvent_Genode(auto_reset);
}
