#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <signal.h>

#include "LiteLib.h"
#include "AlarmThread.h"
#include "ZutexTable.h"
#include "safety_check.h"

AlarmThread::AlarmThread(HostAlarms *alarms)
{
	this->alarms = alarms;
	this->expired = true;
	pthread_mutex_init(&lock, NULL);
	pthread_create(&thread, NULL, start, this);
}

void AlarmThread::reset_alarm(uint64_t next_alarm)
{
	pthread_mutex_lock(&lock);
	this->next_alarm = next_alarm;
	// poke at my internal futex to get the thread to reevaluate the
	// next_alarm field.
	this->expired = false;
	pthread_mutex_unlock(&lock);
	alarm_futex += 1;
	syscall(__NR_futex, &alarm_futex, FUTEX_WAKE, alarm_futex);
}

uint64_t AlarmThread::get_time()
{
	struct timeval tv;
	int rc = gettimeofday(&tv, NULL);
	lite_assert(rc==0);
	return ((uint64_t) tv.tv_sec)*1000000000
				+ ((uint64_t) tv.tv_usec)*1000;
}

void *AlarmThread::start(void *v_this)
{
	((AlarmThread*) v_this)->run();
	return NULL;
}

void AlarmThread::run()
{
	struct timespec timeout_ts, *p_timeout;

	pthread_mutex_lock(&lock);
	uint64_t now = get_time();
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
			// No alarm is waiting. Snooze forever.
			p_timeout = NULL;
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
		}
		pthread_mutex_unlock(&lock);

		syscall(__NR_futex, &alarm_futex, FUTEX_WAIT, alarm_futex_value, p_timeout);

		pthread_mutex_lock(&lock);
		now = get_time();
		if (!expired && now >= next_alarm)
		{
			// Time to wake someone! Woo!
			alarms->signal(alarm_clock);
			expired = true;
		}
	}
}
