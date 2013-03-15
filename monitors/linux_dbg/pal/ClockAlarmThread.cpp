#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <signal.h>

#include "pal_thread.h"
#include "LiteLib.h"
#include "ClockAlarmThread.h"
#include "gsfree_syscall.h"
#include "pal_abi/pal_zutex.h"
#include "xaxInvokeLinux.h"

ClockAlarmThread::ClockAlarmThread(ZLCEmit *ze, HostAlarms *host_alarms)
{
	this->ze = ze;
	this->host_alarms = host_alarms;
	this->expired = true;
	cheesy_lock_init(&alarm_lock);
	pal_thread_create(start, this, 8<<12);
	ZLC_CHATTY(ze, "ClockAlarmThread ctor")
}

void ClockAlarmThread::reset_alarm(uint64_t next_alarm)
{
	cheesy_lock_acquire(&alarm_lock);
	// NB read ABI spec: we can't absorb "idempotent" resets;
	// every reset call must be eventually served by a signal.

//		ZLC_CHATTY(ze, "new request %08x %08x\n",,
//			(uint32_t)(next_alarm>>32), (uint32_t) next_alarm);

	this->next_alarm = next_alarm;

	// poke at my internal futex to get the thread to reevaluate the
	// next_alarm field.
	alarm_futex += 1;
	ZLC_CHATTY(ze, "ClockAlarmThread reset_alarm(time %L, zutex 0x%08x) wakes %08x to val %08x\n",, &next_alarm, host_alarms->get_guest_addr(alarm_clock), &alarm_futex, alarm_futex);
	_gsfree_syscall(__NR_futex, &alarm_futex, FUTEX_WAKE, 1);
	expired = false;

	cheesy_lock_release(&alarm_lock);
}

uint64_t ClockAlarmThread::get_time()
{
	struct timeval tv;
	int rc = _gsfree_syscall(__NR_gettimeofday, &tv, NULL);
	lite_assert(rc==0);
	return ((uint64_t) tv.tv_sec)*1000000000
				+ ((uint64_t) tv.tv_usec)*1000;
}

int ClockAlarmThread::start(void *v_this)
{
	((ClockAlarmThread*) v_this)->run();
	return 0;
}

void ClockAlarmThread::run()
{
	debug_announce_thread();

	struct timespec timeout_ts, *p_timeout;

	uint64_t now = get_time();
	cheesy_lock_acquire(&alarm_lock);
	while (true)
	{
		int alarm_futex_value = alarm_futex;
			// This check protects us from a race where
			// we evaluate the clock, decide to wait forever,
			// someone arrives and resets the alarm,
			// pokes us (if it were a signal, we'd ignore it) and
			// then we follow through on our decision to sleep
			// (but the wake signal got lost). By using a futex,
			// we ensure that we don't actually fall asleep if things
			// have changed since we evaluated the clock.

		if (expired)
		{
			// No alarm is scheduled. Snooze forever.
			p_timeout = NULL;
			ZLC_CHATTY(ze, "ClockAlarmThread no customers @v%08x; sleeping forever\n",, alarm_futex_value);
		}
		else
		{
			// Schedule timeout for requested time.
			uint64_t wait_time = next_alarm - now;
			uint32_t wait_sec = (wait_time / 1000000000LL);
				// can rollover; in which case we just wake up too soon
				// and reevaluate our situation.
			uint32_t wait_nsec = (wait_time - (wait_sec*1000000000LL));

			timeout_ts.tv_sec = wait_sec;
			timeout_ts.tv_nsec = wait_nsec;
			p_timeout = &timeout_ts;
			ZLC_CHATTY(ze, "ClockAlarmThread sleeping for 0x%08x:0x%08x\n",, p_timeout->tv_sec, p_timeout->tv_nsec);
		}
		cheesy_lock_release(&alarm_lock);
		_gsfree_syscall(__NR_futex, &alarm_futex, FUTEX_WAIT, alarm_futex_value, p_timeout);
		cheesy_lock_acquire(&alarm_lock);
		ZLC_CHATTY(ze, "ClockAlarmThread wakes at %L\n",, &now);

		now = get_time();
		if (!expired && now >= next_alarm)
		{
			// Time to wake someone! Woo!
			ZLC_CHATTY(ze, "ClockAlarmThread pokes zutex(0x%08x)\n",, host_alarms->get_guest_addr(alarm_clock));
			host_alarms->signal(alarm_clock);
			expired = true;
		}
		else
		{
			ZLC_CHATTY(ze, "Alarm time not here yet; ClockAlarmThread circles back\n");
		}
	}
	cheesy_lock_release(&alarm_lock);
}

