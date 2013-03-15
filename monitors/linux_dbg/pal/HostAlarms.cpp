#include "HostAlarms.h"
#include "LiteLib.h"
#include "xaxInvokeLinux.h"

HostAlarms::HostAlarms()
{
	lite_memset(&alarms, 0, sizeof(alarms));
}

void HostAlarms::signal(AlarmEnum alarm)
{
	uint32_t *alarm_zutex = get_guest_addr(alarm);
	*alarm_zutex += 1;
	_xil_zutex_wake_inner(alarm_zutex, *alarm_zutex, 1, 0, NULL, 0);
}

#if 0 // dead code
void HostAlarms::signal(AlarmEnum alarm, uint32_t value)
{
	uint32_t *alarm = get_guest_addr(alarm);
	*alarm = value;
	_xil_zutex_wake_inner(alarm, *alarm, 1, 0, NULL, 0);
}
#endif

uint32_t *HostAlarms::get_guest_addr(AlarmEnum alarm)
{
	uint32_t *addr = &((uint32_t*)&alarms)[alarm];
	return addr;
}
