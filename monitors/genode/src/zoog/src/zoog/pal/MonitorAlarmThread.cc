#include <base/printf.h>
#include "MonitorAlarmThread.h"

#include "ZutexTable.h"
#include "HostAlarms.h"
#include "LiteLib.h"

MonitorAlarmThread::MonitorAlarmThread(EventSynchronizerIfc *event_synchronizer, ZoogMonitor::Connection *zoog_monitor)
{
	this->event_synchronizer = event_synchronizer;
	this->network_receive_sequence_number = 0;
	this->ui_event_sequence_number = 0;

	Signal_context_capability network_receive_sigh_cap =
		signal_receiver.manage(&network_receive_context);
	Signal_context_capability ui_receive_sigh_cap =
		signal_receiver.manage(&ui_event_receive_context);

	zoog_monitor->event_receive_sighs(
		network_receive_sigh_cap, ui_receive_sigh_cap);

	start();
}

void MonitorAlarmThread::entry()
{
	while (1)
	{
		Signal s = signal_receiver.wait_for_signal();
		if (s.context() == &network_receive_context)
		{
//			PDBG("alarm_receive_net_buffer");
			network_receive_sequence_number += s.num();
			event_synchronizer->update_event(
				alarm_receive_net_buffer, network_receive_sequence_number);
		}
		else if (s.context() == &ui_event_receive_context)
		{
//			PDBG("alarm_receive_ui_event");
			ui_event_sequence_number += s.num();
			event_synchronizer->update_event(
				alarm_receive_ui_event, ui_event_sequence_number);
		}
		else
		{
			PDBG("unknown context");
			lite_assert(0);	// unknown context
		}
	}
}
