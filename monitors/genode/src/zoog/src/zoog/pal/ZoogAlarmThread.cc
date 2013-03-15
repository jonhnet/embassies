#include "ZoogAlarmThread.h"

#include "ZutexTable.h"
#include "HostAlarms.h"


ZoogAlarmThread::ZoogAlarmThread(ZutexTable *zutex_table, EventSynchronizerIfc *event_synchronizer, SyncFactory *sf)
{
	this->zutex_table = zutex_table;
	this->event_synchronizer = event_synchronizer;
	this->mutex = sf->new_mutex(false);
	this->curr_time = 0;
	this->next_due = 0;
	this->sequence_number = 0;
	start();
}

void ZoogAlarmThread::reset_alarm(uint64_t next_alarm)
{
	mutex->lock();
	this->next_due = next_alarm;
	mutex->unlock();
}

void ZoogAlarmThread::entry()
{
	// This is ridicu-ugly, but for some reason all the code in
	// Genode works this way. There's some sort of nanosecond-y interface
	// in ./os/src/drivers/timer/pistachio/platform_timer.cc, but I can't
	// figure out how to get to it at use it; all the examples I find
	// are this stupid msleep() loop. So don't sweat it for now.
	enum { GRANULARITY_MSECS = 10 };
	
	while (1)
	{
		timer.msleep(GRANULARITY_MSECS);
		mutex->lock();
		curr_time += GRANULARITY_MSECS * ((uint64_t) 1000000);
		if (next_due!=0 && curr_time > next_due)
		{
			next_due = 0;
			sequence_number += 1;
			event_synchronizer->update_event(alarm_clock, sequence_number);
		}
		mutex->unlock();
	}
}

uint64_t ZoogAlarmThread::get_time()
{
	mutex->lock();
	uint64_t result = curr_time;
	mutex->unlock();
	return result;
}
