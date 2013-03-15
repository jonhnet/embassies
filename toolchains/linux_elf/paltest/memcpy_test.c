#include "relocate_this.h"
#include "perf_measure.h"
#include "LiteLib.h"

#define NUMBUFS	1
#define BUFSIZE	(140*1024*1024)

void
elfmain(
	ZoogDispatchTable_v1 *zdt,
	uint32_t elfbase,
	uint32_t rel_offset,
	uint32_t rel_count)
{
	// First things first: fix up relocs so we can use globals & func ptrs
	// and pretty much everything else.
	relocate_this(elfbase, rel_offset, rel_count);

	char *buffer0 = (char*) (zdt->zoog_allocate_memory)(BUFSIZE);
	char *buffer1 = (char*) (zdt->zoog_allocate_memory)(BUFSIZE);
	perf_measure_mark_time(zdt, "start");
	int i;
	for (i=0; i<NUMBUFS; i++)
	{
		lite_memcpy(buffer1, buffer0, BUFSIZE);
	}
	perf_measure_mark_time(zdt, "lite_memcpy_end");
}
