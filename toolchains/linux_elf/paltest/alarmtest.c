#include "paltest.h"
#include "cheesy_snprintf.h"

uint32_t ns_to_ms(uint64_t time_ns)
{
	return ((uint32_t) time_ns)/1000000;
}

void
alarmtest(int seconds)
{
	(g_debuglog)("stderr", "Beginning time & alarm test.\n");
	uint64_t start_time;
	(g_zdt->zoog_get_time)(&start_time);

	uint64_t current_time;
	char msg[100];
	uint32_t *zutex = &(g_zdt->zoog_get_alarms)()->clock;
	int secs;
	for (secs = 0; secs<5; secs++)
	{
//		cheesy_snprintf(msg, sizeof(msg), "Setting up for second %d\n", secs);
//		(g_debuglog)("stderr", msg);
		
		uint64_t target_time = start_time+secs*1000000000LL;
		(g_zdt->zoog_set_clock_alarm)(target_time);

		while (true)
		{
			uint64_t current_time;
			(g_zdt->zoog_get_time)(&current_time);

#if 0
#define TIME_HI(a)		((uint32_t)((a)>>32))
#define TIME_LO(a)		((uint32_t)((a)&0x00000000ffffffffLL))
#define TIME_MINUS(a,b)	((int32_t)(((int64_t)a)-((int64_t)b)))
			cheesy_snprintf(msg, sizeof(msg), "Current time %08x %08x %d ms\n",
				TIME_HI(current_time), TIME_LO(current_time),
				TIME_MINUS(current_time, start_time) / 1000000);
			(g_debuglog)("stderr", msg);
			cheesy_snprintf(msg, sizeof(msg), "Target time  %08x %08x %d ms\n",
				TIME_HI(target_time), TIME_LO(target_time),
				TIME_MINUS(target_time, start_time) / 1000000);
			(g_debuglog)("stderr", msg);
			cheesy_snprintf(msg, sizeof(msg), "current_time - target_time %d ms\n",
				TIME_MINUS(current_time, target_time) / 1000000);
			(g_debuglog)("stderr", msg);
#endif
			if (current_time >= target_time)
			{
//				(g_debuglog)("stderr", "current_time >= target_time\n");
				break;
			}

			cheesy_snprintf(msg, sizeof(msg), "Time now: %d ms Time to go: %d ms zutex: %d\n",
				ns_to_ms(current_time - start_time),
				ns_to_ms(target_time - current_time),
				*zutex);
			(g_debuglog)("stderr", msg);
			ZutexWaitSpec specs[1];
			specs[0].zutex = zutex;
			specs[0].match_val = *zutex;
			(g_zdt->zoog_zutex_wait)(specs, 1);
		}
	}

	(g_zdt->zoog_get_time)(&current_time);
	cheesy_snprintf(msg, sizeof(msg), "Final time: %d ms\n",
		ns_to_ms(current_time - start_time));
	(g_debuglog)("stderr", msg);
}
