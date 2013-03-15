#include "ZClock.h"
#include "pal_abi/pal_extensions.h"
#include "ZLCEmitXdt.h"
#include "zemit.h"

ZASMethods3 ZTimer::ztimer_zas_methods = { NULL, ZTimer::s_check, NULL, NULL };

ZTimer::ZTimer(ZClock *zclock, uint64_t alarm_time)
{
	this->zclock = zclock;
	this->alarm_time = alarm_time;
	this->_tiebreaker = zclock->make_tiebreaker();
	this->fired = false;
	this->private_zutex = 0;

	zas_struct.zas.method_table = &ztimer_zas_methods;
	zas_struct.z_this = this;
	uint32_t ebp;
	asm("mov %%ebp,%0" : "=m"(ebp));
	ZLC_CHATTY(zclock->get_ze(), "Timer 0x%08x ctor (parent eip %08x); set for %08x %08x\n",,
		(uint32_t) this,
		((uint32_t*)ebp)[1],
		(uint32_t)(alarm_time>>32), (uint32_t) alarm_time);

	zclock->activate(this);
}

ZTimer::ZTimer(uint64_t alarm_time, uint32_t _tiebreaker)
{	// key
	this->zclock = NULL;
	this->alarm_time = alarm_time;
	this->_tiebreaker = _tiebreaker;
}

ZTimer::~ZTimer()
{
	if (this->zclock!=NULL)
	{
		if (!fired)
		{
			deactivate();
		}
		ZLC_CHATTY(zclock->get_ze(), "Timer 0x%08x dtor.\n",,
			(uint32_t) this);
	}
}

void ZTimer::deactivate()
{
	lite_assert(!fired);
	fired = true;
	zclock->deactivate(this);
}

ZAS *ZTimer::get_zas()
{
	//ZLC_CHATTY(zclock->get_ze(), "Timer get_zas\n");
	lite_assert(this->zclock!=NULL);	// hey, this is just a key.
	return &zas_struct.zas;
}

bool ZTimer::s_check(ZoogDispatchTable_v1 *zdt, struct _s_zas *zas, ZutexWaitSpec *out_spec)
{
	struct s_zas_struct *zs = (struct s_zas_struct *) zas;
	return zs->z_this->check(out_spec);
}

bool ZTimer::check(ZutexWaitSpec *out_spec)
{
	zclock->discharge_duties(this);

	if (fired)
	{
		// we've already fired and have left the tree;
		// nothing to do.
		ZLC_CHATTY(zclock->get_ze(), "Timer check true; already fired\n");
		return true;
	}

	uint64_t time_now = zclock->get_time_zoog();
	if (time_now >= alarm_time)
	{
		ZLC_CHATTY(zclock->get_ze(), "Timer check just fired\n");
		deactivate();
		return true;
	}

	ZLC_CHATTY(zclock->get_ze(), "Timer check false time now %08x %08x\n",,
		((uint32_t) (time_now>>32)), (uint32_t) time_now);
	ZLC_CHATTY(zclock->get_ze(), "            alarm time     %08x %08x\n",,
		((uint32_t) (alarm_time>>32)), (uint32_t) alarm_time);
	*out_spec = spec;
	return false;
}

ZClock::ZClock(ZoogDispatchTable_v1 *zdt, SyncFactory *sf)
{
	this->zdt = zdt;
	this->alarm_zutex = &((zdt->zoog_get_alarms)()->clock);

	this->mutex = sf->new_mutex(false);
	this->master = NULL;
	this->timer_tree = new ZTimerTree(sf);

	this->ze = new ZLCEmitXdt(zdt, terse);

	this->legacy.legacy_zclock.methods = &legacy_zclock_methods;
	this->legacy.x_this = this;
}

LegacyZTimer *ZClock::legacy_new_timer(LegacyZClock *zclock, const struct timespec *alarm_time_ts)
{
	struct s_legacy *legacy = (struct s_legacy *) zclock;
	uint64_t zoog_time = zoog_time_from_timespec(alarm_time_ts);
	
	ZTimer *timer = new ZTimer(legacy->x_this, zoog_time);
	return (LegacyZTimer*) timer;
}

void ZClock::legacy_read(LegacyZClock *zclock, struct timespec *out_cur_time)
{
	struct s_legacy *legacy = (struct s_legacy *) zclock;
	uint64_t zoog_time = legacy->x_this->get_time_zoog();
	zoog_time_to_timespec(zoog_time, out_cur_time);
}

ZAS *ZClock::legacy_get_zas(LegacyZTimer *legacy_ztimer)
{
	ZTimer *ztimer = (ZTimer *) legacy_ztimer;
	return ztimer->get_zas();
}

void ZClock::legacy_free_timer(LegacyZTimer *legacy_ztimer)
{
	ZTimer *ztimer = (ZTimer *) legacy_ztimer;
	delete ztimer;
}

LegacyZClockMethods ZClock::legacy_zclock_methods =
	{ legacy_new_timer, legacy_read, legacy_get_zas, legacy_free_timer };

ZClock::~ZClock()
{
	// I don't expect this to ever get called, frankly, since
	// there would typically be one ZClock per app, to multiplex the
	// Zoog alarm interface.
	lite_assert(timer_tree->size()==0);
	delete timer_tree;
	delete mutex;
}

//////////////////////////////////////////////////////////////////////////////

void ZClock::activate(ZTimer *timer)
{
	mutex->lock();
	timer_tree->insert(timer);
	_discharge_duties_nolock(timer);
	if (timer_tree->findMin()==timer && master!=timer)
	{
		// we're new, and the earliest timer,
		// and not the master (in which case we'd have
		// just done this), so we need to be sure that the master
		// wakes up when *our* time expires.
		_reset_host_alarm(timer->get_alarm_time());
	}
	mutex->unlock();
}

void ZClock::deactivate(ZTimer *timer)
{
	mutex->lock();
	ZTimer *removed_timer = timer_tree->remove(timer->getKey());
	lite_assert(removed_timer==NULL || removed_timer==timer);
	// If we were the master, delegate the job.
	_discharge_duties_nolock(timer);
	lite_assert(master!=timer);
	mutex->unlock();
}

void ZClock::discharge_duties(ZTimer *timer)
{
	mutex->lock();
	_discharge_duties_nolock(timer);
	mutex->unlock();
}

void ZClock::_reset_host_alarm(uint64_t time)
{
	// Capture the alarm_zutex value. Calling set_clock_alarm()
	// will prod it eventually, and we want to avoid a race where
	// the prod happens, then we later capture the already-prodded
	// value and sleep forever.
	alarm_zutex_match_val = *alarm_zutex;
	(zdt->zoog_set_clock_alarm)(time);
}

void ZClock::_discharge_duties_nolock(ZTimer *timer)
{
	if (master == timer)
	{
		// stop being master (maybe we'll re-accept the role in a moment,
		// but not if we're deactivated)
		master = NULL;
		ZLC_CHATTY(ze, "Timer 0x%08x stops being master...\n",,
			(uint32_t) timer);
	}

	// if we don't end up the master, we'll want to have
	// the spec pointing at the timer's private zutex
	// NB no need for ordering constraints on match_val collection;
	// the thread that wakes this private_zutex will have to wait
	// for the ZClock mutex to do so.
	timer->spec.match_val = timer->private_zutex;
	timer->spec.zutex = &timer->private_zutex;

	ZTimer *min_timer = timer_tree->findMin();
	if (master)
	{
		// well, someone else is responsible; don't do anything
		ZLC_CHATTY(ze, "Timer 0x%08x notices 0x%08x is already master.\n",,
			(uint32_t) timer,
			(uint32_t) master);
	}
	else if (min_timer==timer)
	{
		// become master
		ZLC_CHATTY(ze, "Timer 0x%08x is min; becomes master.\n",,
			(uint32_t) timer);

		master = timer;

		// since we're the min time, the host alarm should be set to
		// us. This can happen either because we're the first guy into
		// an empty tree, or because we just showed up at the front,
		// and the departing master woke us up so we could do this.
		_reset_host_alarm(timer->get_alarm_time());

		timer->spec.match_val = alarm_zutex_match_val;
		timer->spec.zutex = alarm_zutex;
	}
	else if (min_timer!=NULL)
	{
		// there's no master, but there are guys in the tree.
		// wake min; he'll discharge_duties and become the master
		ZLC_CHATTY(ze, "Timer 0x%08x wakes min_timer 0x%08x.\n",,
			(uint32_t) timer, (uint32_t) min_timer);

		min_timer->private_zutex++;
		(zdt->zoog_zutex_wake)(
			&min_timer->private_zutex,
			min_timer->private_zutex,
			1,
			0,
			NULL,
			0);
	}
	else
	{
		// tree is empty; no master needed.
		ZLC_CHATTY(ze, "Timer 0x%08x notices tree is empty.\n",,
			(uint32_t) timer);
	}
}

// TODO there's a word-ripping race here, but I really don't want to lock
// this just yet.
uint64_t ZClock::get_time_zoog()
{
	uint64_t result;
	(zdt->zoog_get_time)(&result);
	return result;
}

uint32_t ZClock::make_tiebreaker()
{
	mutex->lock();
	uint32_t result = s_tiebreaker;
	s_tiebreaker += 1;
	mutex->unlock();
	return result;
}

uint32_t ZClock::s_tiebreaker = 1;

