#include "pal_abi/pal_extensions.h"
#include "perf_measure.h"

PerfMeasure::PerfMeasure(ZoogDispatchTable_v1 *zdt)
	: zdt(zdt)
{
	this->debug_logfile_append = (debug_logfile_append_f *)
		(zdt->zoog_lookup_extension)("debug_logfile_append");
}

void PerfMeasure::mark_time(const char *event_label)
{
	uint64_t event_time;
	(zdt->zoog_get_time)(&event_time);
	char buf[100];
	cheesy_snprintf(buf, sizeof(buf), "mark_time %L %s\n", &event_time, event_label);
	debug_logfile_append("perf", buf);
}

void perf_measure_mark_time(ZoogDispatchTable_v1 *zdt, const char *event_label)
{
	PerfMeasure(zdt).mark_time(event_label);
}
