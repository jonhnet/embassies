#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <signal.h>

#include "pal_thread.h"
#include "LiteLib.h"
#include "MonitorAlarmThread.h"
#include "gsfree_syscall.h"
#include "pal_abi/pal_zutex.h"
#include "xaxInvokeLinux.h"

MonitorAlarmThread::MonitorAlarmThread(ZLCEmit *ze, HostAlarms *host_alarms)
{
	this->ze = ze;
	this->host_alarms = host_alarms;
	pal_thread_create(start, this, 8<<12);
	ZLC_CHATTY(ze, "MonitorAlarmThread ctor")
}

int MonitorAlarmThread::start(void *v_this)
{
	((MonitorAlarmThread*) v_this)->run();
	return 0;
}

void MonitorAlarmThread::run()
{
	debug_announce_thread();

	uint32_t old_sequence_numbers[alarm_count];
	uint32_t new_sequence_numbers[alarm_count];
	while (true)
	{
		xil_alarm_poll(old_sequence_numbers, new_sequence_numbers);

		int alarm;
		for (alarm=0; alarm<alarm_count; alarm++)
		{
			if (new_sequence_numbers[alarm] != old_sequence_numbers[alarm])
			{
				ZLC_CHATTY(ze, "MonitorAlarmThread pokes alarm %d\n",, alarm);
				host_alarms->signal((AlarmEnum) alarm);
				old_sequence_numbers[alarm] = new_sequence_numbers[alarm];
			}
		}
	}
}
