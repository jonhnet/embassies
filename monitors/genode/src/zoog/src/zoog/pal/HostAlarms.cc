#include "HostAlarms.h"
#include <util/string.h>

using namespace Genode;

HostAlarms::HostAlarms(ZutexTable *zutex_table)
{
	this->zutex_table = zutex_table;
	memset(&the_alarms, 0, sizeof(the_alarms));
	the_alarms_ary = (uint32_t*) &the_alarms;
}

void HostAlarms::signal(AlarmEnum alarm)
{
	uint32_t *host_alarm_ptr = &the_alarms_ary[alarm];

	(*host_alarm_ptr) += 1;
	zutex_table->wake(get_guest_addr(alarm), 1);
}

void HostAlarms::update_event(AlarmEnum alarm, uint32_t sequence_number)
{
	uint32_t *host_alarm_ptr = &the_alarms_ary[alarm];
	(*host_alarm_ptr) = sequence_number;
	zutex_table->wake(get_guest_addr(alarm), 1);
}

// This function's kind of silly on this host, where there's no guest/host
// distinction. But leaving it here to it's easier to see the refactoring
// when we get there.
uint32_t HostAlarms::get_guest_addr(AlarmEnum alarm)
{
	uint32_t *host_alarm_ptr = &the_alarms_ary[alarm];
	return (uint32_t) host_alarm_ptr;
}

