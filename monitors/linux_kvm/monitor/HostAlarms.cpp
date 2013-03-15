#include "HostAlarms.h"

HostAlarms::HostAlarms(ZutexTable *zutex_table, MemSlot *host_alarms_page)
{
	this->zutex_table = zutex_table;
	this->host_alarms_page = host_alarms_page;
	this->host_view = (ZoogHostAlarms *) host_alarms_page->get_host_addr();
}

void HostAlarms::signal(AlarmEnum alarm)
{
	uint32_t *host_zutexes = (uint32_t*) host_alarms_page->get_host_addr();
	uint32_t *host_alarm_ptr = &host_zutexes[alarm];

	(*host_alarm_ptr) += 1;
	zutex_table->wake(get_guest_addr(alarm), 1);
}

void HostAlarms::update_event(AlarmEnum alarm, uint32_t sequence_number)
{
	uint32_t *host_zutexes = (uint32_t*) host_alarms_page->get_host_addr();
	uint32_t *host_alarm_ptr = &host_zutexes[alarm];
	(*host_alarm_ptr) = sequence_number;
	zutex_table->wake(get_guest_addr(alarm), 1);
}

uint32_t HostAlarms::get_guest_addr(AlarmEnum alarm)
{
	return host_alarms_page->get_guest_addr()+sizeof(uint32_t)*alarm;
}
